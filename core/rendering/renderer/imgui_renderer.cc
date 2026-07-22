#include "imgui_renderer.h"
#include <imgui.h>
#include "core/math/mat4.h"
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_engine.h"
#include "renderer.h"

namespace Seed {
void ImguiRenderer::init(Window *window) {
    ImGuiIO &io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr &&
              "Already initialized a renderer backend!");
    io.BackendRendererName = "imgui_impl_seed";

    fd.vertex =
        RHI::alloc_vertex(System::gDefaultStorage->gui_desc.get_stride(), 0,
                          UpdateFrequence::PERDRAW, nullptr);
    fd.indices = RHI::alloc_index(std::vector<u16>(), UpdateFrequence::PERDRAW);
    // Build texture atlas
    create_font_material();

    gui_pass.setup(window);
}

void ImguiRenderer::create_font_material() {
    ImGuiIO &io = ImGui::GetIO();

    u8 *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    Ref<Texture> font_tex(TextureType::TEXTURE_2D, width, height,
                          PixelFormat::RGBA, SamplerProperty{}, pixels);
    RenderBlendState blend_state = {
        .blend_on = true,
        .func = BlendFunc::create(
            BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA,
            BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA)};
    io.Fonts->SetTexID((ImTextureID)(u64)font_tex->get_handle());
    fd.font_material.create(System::gDefaultStorage->gui_shader,
                            RenderRasterizerState{}, RenderDepthStencilState{},
                            blend_state);
    fd.font_material->set_texture("u_texture", font_tex);
}

void ImguiRenderer::preprocess() {
    ImGuiIO &io = ImGui::GetIO();
    if (!io.Fonts->IsBuilt()) {
        create_font_material();
    }
}

void ImguiRenderer::_process(RenderCommandDispatcher &dp) {
    gui_pass.draw(dp, fd);
}
void ImguiRenderer::cleanup() {}
void ImguiRenderer::GUIPass::execute(RenderCommandDispatcher &dp,
                                     Viewport &viewport, FrameData &fd) {
    FrameGlobal &g_frame = System::gRenderEngine->get_frame_global();
    ImDrawData *draw_data = ImGui::GetDrawData();
    RectF view_rect = viewport.get_actual_dimension();
    int fb_width = view_rect.w;
    int fb_height = view_rect.h;
    if (fb_width <= 0 || fb_height <= 0) return;

    f32 L = 0;
    f32 R = fb_width;
    f32 T = 0;
    f32 B = fb_height;
    const Mat4 ortho_projection = {
        Vec4{2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        Vec4{0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        Vec4{0.0f, 0.0f, -1.0f, 0.0f},
        Vec4{(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };
    Mat4 proj = ortho_projection.transpose();
    RHI::update(g_frame.projection, 0, sizeof(Mat4), &proj);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off =
        draw_data->DisplayPos;  // (0,0) unless using multi-viewports
    ImVec2 clip_scale =
        draw_data->FramebufferScale;  // (1,1) unless using retina display
                                      // which are often (2,2)
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList *draw_list = draw_data->CmdLists[n];
        u32 vtx_buffer_size =
            draw_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
        u32 idx_buffer_size =
            draw_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);
        dp.push_buffer(fd.vertex, vtx_buffer_size, draw_list->VtxBuffer.Data);
        dp.push_buffer(fd.indices, idx_buffer_size, draw_list->IdxBuffer.Data);

        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback
                // value used by the user to request the renderer to reset
                // render state.)
                if (pcmd->UserCallback != ImDrawCallback_ResetRenderState)
                    pcmd->UserCallback(draw_list, pcmd);
            } else {
                // Project scissor/clipping rectangles into framebuffer
                // space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_min.x < 0.0f) clip_min.x = 0.0f;
                if (clip_min.y < 0.0f) clip_min.y = 0.0f;
                if (clip_max.x > fb_width) clip_max.x = (float)fb_width;
                if (clip_max.y > fb_height) clip_max.y = (float)fb_height;
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;
                RenderDrawDataBuilder builder;
                builder.bind_texture(0, pcmd->TextureId);
                builder.set_viewport(view_rect.x, view_rect.y, fb_width,
                                     fb_height);
                builder.set_scissor(clip_min.x, clip_min.y,
                                    clip_max.x - clip_min.x,
                                    clip_max.y - clip_min.y);
                builder.bind_vertex(fd.vertex);
                builder.bind_description(&System::gDefaultStorage->gui_desc);
                builder.bind_index(fd.indices);
                builder.set_draw_vertex(draw_list->VtxBuffer.Size,
                                        pcmd->VtxOffset);
                builder.set_draw_index(pcmd->ElemCount, pcmd->IdxOffset);
                dp.render(builder, RenderPrimitiveType::TRIANGLES,
                          fd.font_material->get_pipeline(), 0);
            }
        }
    }
}
}  // namespace Seed