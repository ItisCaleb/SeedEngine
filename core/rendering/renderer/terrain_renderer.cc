#include "terrain_renderer.h"
#include "core/resource/resource_loader.h"
#include <spdlog/spdlog.h>
#include "core/engine.h"

namespace Seed {
void TerrainRenderer::init() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    try {
        this->terrain_shader = loader->loadShader(
            "assets/terrain.vert", "assets/terrain.frag", "",
            "assets/terrain.tessctl", "assets/terrain.tesseval");
    } catch (std::exception &e) {
        SPDLOG_ERROR("Error loading Shader: {}", e.what());
        exit(1);
    }

    vertices_desc.add_attr(0, VertexAttributeType::FLOAT, 2, 0);
    vertices_desc.add_attr(1, VertexAttributeType::FLOAT, 2, 0);
    auto model = Mat4::translate_mat({0, 0, 0}).transpose();
    model_const_rc.alloc_constant("TerrainMatrices", sizeof(Mat4), &model);
}

void TerrainRenderer::preprocess() {}

void TerrainRenderer::process(u8 layer) {
    RenderCommandDispatcher dp(layer);
    Ref<Terrain> terrain =
        SeedEngine::get_instance()->get_world()->get_terrain();
    if (terrain.is_null()) {
        return;
    }
    RenderDispatchData data = dp.generate_render_data(
        &terrain->vertices, &this->vertices_desc, RenderPrimitiveType::PATCHES,
        terrain->terrain_mat);
    dp.render(&data, &terrain_shader);
}
void TerrainRenderer::cleanup() {}
}  // namespace Seed