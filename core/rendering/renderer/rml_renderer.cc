#include "rml_renderer.h"
#include "core/gui/rml_interface.h"
#include "core/math/mat4.h"
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/default_storage.h"
#include <algorithm>

namespace Seed {

void RmlRenderer::init(Window *window) {
    instance = this;
    this->window = window;
    create_material();
    gui_pass.setup(window);
}

void RmlRenderer::create_material() {
    RenderBlendState blend_state = {
        .blend_on = true,
        .func = BlendFunc::create(
            BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA,
            BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA)};
    fd.material.create(DS::get_instance()->rml_shader, RenderRasterizerState{},
                       RenderDepthStencilState{}, blend_state);
    fd.material->set_texture("u_texture", DS::get_instance()->white_texture);
}

void RmlRenderer::preprocess() {}

void RmlRenderer::_process(RenderCommandDispatcher &dp) {
    gui_pass.draw(dp, fd);
}

void RmlRenderer::cleanup() {}

void RmlRenderer::GUIPass::execute(RenderCommandDispatcher &dp,
                                   Viewport &viewport, FrameData &fd) {
    SeedRmlRenderInterface *rml = SeedRmlRenderInterface::get_instance();
    if (!rml) return;

    RectF view_rect = viewport.get_actual_dimension();
    i32 fb_width = (i32)view_rect.w;
    i32 fb_height = (i32)view_rect.h;
    if (fb_width <= 0 || fb_height <= 0) return;

    FrameGlobal &g_frame = RenderEngine::get_instance()->get_frame_global();
    f32 L = 0;
    f32 R = (f32)fb_width;
    f32 T = 0;
    f32 B = (f32)fb_height;
    const Mat4 ortho_projection = {
        Vec4{2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        Vec4{0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        Vec4{0.0f, 0.0f, -1.0f, 0.0f},
        Vec4{(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };
    Mat4 proj = ortho_projection.transpose();
    RHI::update(g_frame.projection, 0, sizeof(Mat4), &proj);

    for (const RmlDrawCommand &cmd : rml->get_commands()) {
        const RmlGeometry *geometry = rml->get_geometry(cmd.geometry);
        if (!geometry || geometry->vertex_count == 0 ||
            geometry->index_count == 0)
            continue;

        f32 scissor_x = 0;
        f32 scissor_y = 0;
        f32 scissor_w = (f32)fb_width;
        f32 scissor_h = (f32)fb_height;
        if (cmd.scissor_enabled) {
            i32 left = std::max(0, cmd.scissor.Left());
            i32 top = std::max(0, cmd.scissor.Top());
            i32 right = std::min(fb_width, cmd.scissor.Right());
            i32 bottom = std::min(fb_height, cmd.scissor.Bottom());
            if (right <= left || bottom <= top) continue;
            scissor_x = (f32)left;
            scissor_y = (f32)top;
            scissor_w = (f32)(right - left);
            scissor_h = (f32)(bottom - top);
        }

        TextureHandle texture = rml->get_texture(cmd.texture);
        if (texture == NULL_HANDLE)
            texture = DS::get_instance()->white_texture->get_handle();


        RenderDrawDataBuilder builder;
        builder.bind_texture(0, texture);

        /* push constants */
        builder.push_constant(cmd.translation);
        /* padding */
        builder.push_constant(Vec2{0, 0});
        builder.push_constant(cmd.transform);


        builder.set_viewport(view_rect.x, view_rect.y, fb_width, fb_height);
        builder.set_scissor(scissor_x, scissor_y, scissor_w, scissor_h);
        builder.bind_vertex(geometry->vertex);
        builder.bind_description(&DS::get_instance()->gui_desc);
        builder.bind_index(geometry->index);
        builder.set_draw_vertex(geometry->vertex_count, 0);
        builder.set_draw_index(geometry->index_count, 0);
        dp.render(builder, RenderPrimitiveType::TRIANGLES,
                  fd.material->get_pipeline(), 0);
    }
}

}  // namespace Seed