#include "debug_pass.h"
#include "core/resource.h"
#include <spdlog/spdlog.h>

namespace Seed {
void DebugPass::init() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    try {
        this->shader = loader->loadShader(
            "assets/debug.vert", "assets/debug.frag", "assets/debug.gs");
    } catch (std::exception &e) {
        spdlog::error("Error loading Shader: {}", e.what());
        exit(1);
    }
    RenderResource::register_resource("Debug", this->shader);

    std::vector<VertexAttribute> aabb_attrs = {
        {.layout_num = 0, .size = 3, .stride = sizeof(AABB)},
        {.layout_num = 1, .size = 3, .stride = sizeof(AABB)}};
    aabb_desc_rc.alloc_vertex_desc(aabb_attrs);
    aabb_vertices_rc.alloc_vertex(sizeof(AABB), 0, NULL);
}
void DebugPass::process(RenderCommandDispatcher &dp, u64 sort_key) {
    dp.begin(sort_key);
    dp.use(&aabb_vertices_rc);
    dp.use(&aabb_desc_rc);
    dp.update(&aabb_vertices_rc, 0, sizeof(AABB) * entity_aabb.size(), (void*)entity_aabb.data());
    dp.render(&this->shader, RenderPrimitiveType::POINTS, 0, false);
    dp.end();
}
}  // namespace Seed