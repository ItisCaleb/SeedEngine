#include "imgui_renderer.h"
#include <imgui.h>
#include "core/math/mat4.h"
#include "core/engine.h"

namespace Seed {
void ImguiRenderer::init() {
    ImGuiIO &io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr &&
              "Already initialized a renderer backend!");
    ImguiData *bd = IM_NEW(ImguiData)();
    io.BackendRendererUserData = (void *)bd;
    io.BackendRendererName = "imgui_impl_seed";

    desc.add_attr(0, VertexAttributeType::FLOAT, 2, 0);
    desc.add_attr(1, VertexAttributeType::FLOAT, 2, 0);
    desc.add_attr(2, VertexAttributeType::UNSIGNED_BYTE, 4, 0, true);
    bd->vertex.alloc_vertex(desc.get_stride(), 0, nullptr);
    bd->vertex.alloc_index(std::vector<u16>{});

    const char *vertex_shader =
        "#version 410 core\n"
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "layout (std140) uniform GUIProjMtx {\n"
        "   mat4 ProjMtx;\n"
        "};\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const char *fragment_shader =
        "#version 410 core\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D u_texture[8];\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(u_texture[0], Frag_UV.st);\n"
        "}\n";

    Ref<Shader> shader(vertex_shader, fragment_shader);

    // Build texture atlas
    u8 *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    Ref<Texture> font_tex(TextureType::TEXTURE_2D, width, height, pixels);
    font_mat.create();
    font_mat->set_texture_map(Material::TextureMapType::DIFFUSE, font_tex);

    gui_proj.alloc_constant("GUIProjMtx", sizeof(Mat4), nullptr);
    /* create pipeline */
    RenderRasterizerState rst_state;
    RenderDepthStencilState depth_state;
    RenderBlendState blend_state = {
        .blend_on = true,
        .func = BlendFunc::create(
            BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA,
            BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA)};

    this->gui_pipeline.create(shader, &desc, RenderPrimitiveType::TRIANGLES,
                              rst_state, depth_state, blend_state);
}  // namespace Seed
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
    RenderCommandDispatcher dp(layer);
    DEBUG_DISPATCH(dp);

    void *mat = dp.map_buffer(gui_proj, 0, sizeof(ortho_projection));
    memcpy(mat, ortho_projection, sizeof(ortho_projection));
}

ImguiRenderer::ImguiData *ImguiRenderer::get_imgui_data() {
    return ImGui::GetCurrentContext()
               ? (ImguiData *)ImGui::GetIO().BackendRendererUserData
               : nullptr;
}
void ImguiRenderer::process() {
    RenderCommandDispatcher dp(layer);
    DEBUG_DISPATCH(dp);
    ImDrawData *draw_data = ImGui::GetDrawData();
    ImguiData *bd = get_imgui_data();
    int fb_width =
        (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height =
        (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
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
        dp.update_buffer(bd->vertex.get_vertices(), 0, vtx_buffer_size,
                         draw_list->VtxBuffer.Data);
        bd->vertex.set_vertices_cnt(draw_list->VtxBuffer.Size);
        dp.update_buffer(bd->vertex.get_indices(), 0, idx_buffer_size,
                         draw_list->IdxBuffer.Data);
        bd->vertex.set_indices_cnt(draw_list->IdxBuffer.Size);

        dp.begin_draw();

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
                RenderDrawData imgui_data =
                    dp.generate_render_data(bd->vertex, this->font_mat);
                dp.draw_set_viewport(imgui_data, 0, 0, fb_width, fb_height);
                dp.draw_set_scissor(
                    imgui_data, clip_min.x, fb_height - clip_max.y,
                    clip_max.x - clip_min.x, clip_max.y - clip_min.y);
                dp.render(&imgui_data, gui_pipeline, 0, pcmd->ElemCount,
                          pcmd->IdxOffset * sizeof(ImDrawIdx));
            }
        }
        dp.end_draw();
        Window *window = SeedEngine::get_instance()->get_window();

        dp.set_viewport(0, 0, window->get_width(), window->get_height());
        dp.cancel_scissor();
    }
}
void ImguiRenderer::cleanup() {}
}  // namespace Seed