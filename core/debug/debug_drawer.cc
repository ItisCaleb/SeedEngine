#include "debug_drawer.h"
#include "core/resource/resource_loader.h"

namespace Seed {
void DebugDrawer::draw_line(Vec3 from, Vec3 to, Color color) {
    this->line_vertices.push_back(DebugVertex{from, color});
    this->line_vertices.push_back(DebugVertex{to, color});
}
void DebugDrawer::draw_triangle(Vec3 v1, Vec3 v2, Vec3 v3, Color color) {
    this->triangle_vertices.push_back(DebugVertex{v1, color});
    this->triangle_vertices.push_back(DebugVertex{v2, color});
    this->triangle_vertices.push_back(DebugVertex{v3, color});
    u32 index = this->triangle_indices.size();
    this->triangle_indices.push_back(index);
    this->triangle_indices.push_back(index + 1);
    this->triangle_indices.push_back(index + 2);
}

DebugDrawer::DebugDrawer() {
    instance = this;
    ResourceLoader *loader = ResourceLoader::get_instance();
    Ref<Shader> debug_shader = loader->load_shader("assets/debug.vert", "assets/debug.frag");
    debug_desc.add_attr(0, VertexAttributeType::FLOAT, 3, 0);
    debug_desc.add_attr(1, VertexAttributeType::UNSIGNED_BYTE, 4, 0);
    debug_mat.create(debug_shader);
    RenderRasterizerState rst;
    rst.poly_mode = PolygonMode::LINE;
    RenderDepthStencilState depth;
    depth.depth_on = true;
    RenderBlendState blend;
    blend.blend_on = true;
    debug_mat->set_rasterizer_state(rst);
    debug_mat->set_depth_state(depth);
    debug_mat->set_blend_state(blend);
}
}  // namespace Seed
