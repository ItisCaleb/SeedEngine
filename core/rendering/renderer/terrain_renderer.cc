#include "terrain_renderer.h"
#include "core/resource/resource_loader.h"
#include <spdlog/spdlog.h>
#include "core/engine.h"

namespace Seed {
void TerrainRenderer::init() {
    auto model = Mat4::translate_mat({0, 0, 0}).transpose();
    model_const_rc.alloc_constant("TerrainMatrices", sizeof(Mat4), &model);
}

void TerrainRenderer::preprocess() {}

void TerrainRenderer::process() {
    Window *window = SeedEngine::get_instance()->get_window();
    RenderCommandDispatcher dp(layer);
    DEBUG_DISPATCH(dp);

    Ref<Terrain> terrain =
        SeedEngine::get_instance()->get_world()->get_terrain();
    if (terrain.is_null()) {
        return;
    }

    RenderDrawDataBuilder builder =
        dp.generate_render_data(ref_cast<Material>(terrain->terrain_mat));
    builder.bind_vertex_data(terrain->vertices, 0);
    builder.bind_description(DS::get_instance()->get_terrain_desc());

    dp.render(builder, RenderPrimitiveType::PATCHES,
              terrain->terrain_mat->get_pipeline(), 0);
}
void TerrainRenderer::cleanup() {}
}  // namespace Seed