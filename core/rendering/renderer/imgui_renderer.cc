#include "imgui_renderer.h"
#include <imgui.h>
#include "core/math/mat4.h"
#include "core/engine.h"

namespace Seed {
void ImguiRenderer::init() {
    instance = this;
    ImGuiIO &io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr &&
              "Already initialized a renderer backend!");
    ImguiData *bd = IM_NEW(ImguiData)();
    io.BackendRendererUserData = (void *)bd;
    io.BackendRendererName = "imgui_impl_seed";
    bd->vertex.alloc_vertex(DS::get_instance()->gui_desc.get_stride(), 0,
                            nullptr);
    bd->indices.alloc_index(std::vector<u16>());

    // Build texture atlas
    Ref<Texture> atlas = create_font_texture();

    RenderBlendState blend_state = {
        .blend_on = true,
        .func = BlendFunc::create(
            BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA,
            BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA)};
    font_mat.create(DS::get_instance()->gui_shader);
    font_mat->add_texture_unit(atlas);
    font_mat->set_blend_state(blend_state);
    /* create pipeline */
    font_mat->build_pipeline();
    gui_proj.alloc_constant("GUIProjMtx", sizeof(Mat4), nullptr);
}

Ref<Texture> ImguiRenderer::create_font_texture() {
    ImGuiIO &io = ImGui::GetIO();

    u8 *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    Ref<Texture> font_tex(TextureType::TEXTURE_2D, width, height,
                          PixelFormat::RGBA, pixels);
    return font_tex;
}

void ImguiRenderer::new_frame() {
    ImGuiIO &io = ImGui::GetIO();
    if (!io.Fonts->IsBuilt()) {
        Ref<Texture> atlas = create_font_texture();
        font_mat->set_texture_unit(0, atlas);
    }
}

void ImguiRenderer::preprocess() {
    ImDrawData *draw_data = ImGui::GetDrawData();

    f32 L = draw_data->DisplayPos.x;
    f32 R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    f32 T = draw_data->DisplayPos.y;
    f32 B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    const f32 ortho_projection[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };
    RenderCommandDispatcher dp;

    RenderUpdateData *upd = dp.map_buffer(gui_proj, 0, sizeof(ortho_projection),
                                          current_sort_key());
    memcpy(upd->get_buffer(), ortho_projection, sizeof(ortho_projection));
    upd->set_filled();
}

ImguiRenderer::ImguiData *ImguiRenderer::get_imgui_data() {
    return ImGui::GetCurrentContext()
               ? (ImguiData *)ImGui::GetIO().BackendRendererUserData
               : nullptr;
}
void ImguiRenderer::process(Viewport &viewport) {
    RenderCommandDispatcher dp;
    dp.begin_scope("Imgui", current_sort_key());
    ImDrawData *draw_data = ImGui::GetDrawData();
    ImguiData *bd = get_imgui_data();
    RectF view_rect = viewport.get_actual_dimension();
    int fb_width = view_rect.w;
    int fb_height = view_rect.h;

    if (fb_width <= 0 || fb_height <= 0) return;

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off =
        draw_data->DisplayPos;  // (0,0) unless using multi-viewports
    ImVec2 clip_scale =
        draw_data->FramebufferScale;  // (1,1) unless using retina display which
                                      // are often (2,2)
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList *draw_list = draw_data->CmdLists[n];
        u32 vtx_buffer_size =
            draw_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
        u32 idx_buffer_size =
            draw_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);
        dp.update_buffer(bd->vertex, 0, vtx_buffer_size,
                         draw_list->VtxBuffer.Data, current_sort_key());
        dp.update_buffer(bd->indices, 0, idx_buffer_size,
                         draw_list->IdxBuffer.Data, current_sort_key());

        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value
                // used by the user to request the renderer to reset render
                // state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    this->preprocess();
                else
                    pcmd->UserCallback(draw_list, pcmd);
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;
                RenderDrawDataBuilder builder =
                    dp.generate_render_data(this->font_mat);
                builder.set_viewport(view_rect.x, view_rect.y, fb_width,
                                     fb_height);
                builder.set_scissor(clip_min.x, fb_height - clip_max.y,
                                    clip_max.x - clip_min.x,
                                    clip_max.y - clip_min.y);
                builder.bind_vertex(bd->vertex);
                builder.bind_description(&DS::get_instance()->gui_desc);
                builder.bind_index(bd->indices);
                builder.set_draw_vertex(draw_list->VtxBuffer.Size, 0);
                builder.set_draw_index(pcmd->ElemCount,
                                       pcmd->IdxOffset * sizeof(ImDrawIdx));
                dp.render(builder, RenderPrimitiveType::TRIANGLES,
                          font_mat->get_pipeline(), current_sort_key());
            }
        }
    }
    dp.end_scope(next_sort_key());
    RenderStateDataBuilder builder;
    builder.set_scissor(view_rect.x, view_rect.y, fb_width, fb_height);
    dp.set_states(builder, current_sort_key());
}
void ImguiRenderer::cleanup() { this->seq = 0; }
}  // namespace Seed