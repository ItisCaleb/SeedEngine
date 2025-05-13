#include "terrain_renderer.h"
#include "core/resource/resource_loader.h"
#include <spdlog/spdlog.h>
#include "core/engine.h"

namespace Seed {
void TerrainRenderer::init() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    Ref<Shader> terrain_shader =
        loader->load_shader("assets/terrain.vert", "assets/terrain.frag", "",
                            "assets/terrain.tesc", "assets/terrain.tese");

    vertices_desc.add_attr(0, VertexAttributeType::FLOAT, 2, 0);
    vertices_desc.add_attr(1, VertexAttributeType::FLOAT, 2, 0);

    RenderRasterizerState rst_state = {
        .patch_control_points = 4
    };
    RenderDepthStencilState depth_state = {.depth_on = true};
    RenderBlendState blend_state;
    terrain_pipeline.create(terrain_shader, &vertices_desc,
                            RenderPrimitiveType::PATCHES, rst_state,
                            depth_state, blend_state);

    auto model = Mat4::translate_mat({0, 0, 0}).transpose();
    model_const_rc.alloc_constant("TerrainMatrices", sizeof(Mat4), &model);
}

void TerrainRenderer::preprocess() {}

void TerrainRenderer::process() {
    Window *window = SeedEngine::get_instance()->get_window();
    RenderCommandDispatcher dp(layer);
    Ref<Terrain> terrain =
        SeedEngine::get_instance()->get_world()->get_terrain();
    if (terrain.is_null()) {
        return;
    }
    dp.begin_draw();

    RenderDrawData data =
        dp.generate_render_data(terrain->vertices, terrain->terrain_mat);
    dp.draw_set_viewport(data, 0, 0, window->get_width(), window->get_height());

    dp.render(&data, terrain_pipeline, 0);
    dp.end_draw();
}
void TerrainRenderer::cleanup() {}
}  // namespace Seed