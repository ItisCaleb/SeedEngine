#include "post_renderer.h"
#include "core/rendering/api/render_engine.h"
#include <spdlog/spdlog.h>
#include "core/resource/default_storage.h"

namespace Seed {
void PostRenderer::init() {
    post_mat.create(DS::get_instance()->post_shader);
    auto rt = ref_cast<MultiRenderTarget>(
        RenderEngine::get_instance()->get_render_target("default"));
    if (rt.is_null()) {
        SPDLOG_WARN("Can't get default render target");
        return;
    }
    post_mat->add_texture_unit(rt->get_color(0).texture);
}

void PostRenderer::process(Viewport &viewport){
    RenderCommandDispatcher dp;
    dp.begin_scope("POST Rendering", current_sort_key());
    auto builder = dp.generate_render_data(post_mat);
    builder.bind_vertex_data(DS::get_instance()->post_data);
    dp.render(builder, RenderPrimitiveType::TRIANGLES, post_mat->get_pipeline(), current_sort_key());
    dp.end_scope(next_sort_key());
}

void PostRenderer::preprocess(){
    
}

void PostRenderer::cleanup(){
    this->seq = 0;
}

}  // namespace Seed