#include "debug_drawer.h"
#include "core/resource/resource_loader.h"

namespace Seed {
void DebugDrawer::draw_line(Vec3 from, Vec3 to, Color color) {
    this->line_vertices.push_back(DebugVertex{from, color});
    this->line_vertices.push_back(DebugVertex{to, color});
}
void DebugDrawer::draw_triangle(Vec3 v1, Vec3 v2, Vec3 v3, Color color) {
    u32 index = this->triangle_vertices.size();
    this->triangle_vertices.push_back(DebugVertex{v1, color});
    this->triangle_vertices.push_back(DebugVertex{v2, color});
    this->triangle_vertices.push_back(DebugVertex{v3, color});
    this->triangle_indices.push_back(index);
    this->triangle_indices.push_back(index + 1);
    this->triangle_indices.push_back(index + 2);
}

void DebugDrawer::draw_triangles(const std::vector<Vec3> vertices,
                                 const std::vector<u32> indices, Color color) {
    if (indices.size() % 3 != 0) {
        SPDLOG_WARN("Indices must be multiple of 3.");
        return;
    }
    u32 index = this->triangle_vertices.size();
    for (Vec3 v : vertices) {
        this->triangle_vertices.push_back(DebugVertex{v, color});
    }
    for (u32 i : indices) {
        this->triangle_indices.push_back(index + i);
    }
}

void DebugDrawer::draw_frustum(const Frustum &frustum, Color color) {
    // planes: [near, far, left, right, top, bottom]

    Vec3 near_tl = plane_intersect(frustum.near, frustum.left, frustum.top);
    Vec3 near_tr = plane_intersect(frustum.near, frustum.right, frustum.top);
    Vec3 near_bl = plane_intersect(frustum.near, frustum.left, frustum.bottom);
    Vec3 near_br = plane_intersect(frustum.near, frustum.right, frustum.bottom);

    Vec3 far_tl = plane_intersect(frustum.far, frustum.left, frustum.top);
    Vec3 far_tr = plane_intersect(frustum.far, frustum.right, frustum.top);
    Vec3 far_bl = plane_intersect(frustum.far, frustum.left, frustum.bottom);
    Vec3 far_br = plane_intersect(frustum.far, frustum.right, frustum.bottom);

    std::vector<Vec3> vertices = {near_tl, near_tr, near_br, near_bl,
                                  far_tl,  far_tr,  far_br,  far_bl};

    std::vector<uint32_t> indices = {// Near face
                                     0, 1, 2, 0, 2, 3,
                                     // Far face
                                     4, 6, 5, 4, 7, 6,
                                     // Left face
                                     4, 0, 3, 4, 3, 7,
                                     // Right face
                                     5, 6, 2, 5, 2, 1,
                                     // Top face
                                     4, 5, 1, 4, 1, 0,
                                     // Bottom face
                                     7, 3, 2, 7, 2, 6};

    draw_triangles(vertices, indices, color);
}

void DebugDrawer::draw_aabb(const AABB &aabb, Color color) {
    Vec3 c = aabb.center;
    Vec3 e = aabb.ext;

    // 8 corners
    std::vector<Vec3> v(8);
    v[0] = c + Vec3{-e.x, -e.y, -e.z};
    v[1] = c + Vec3{e.x, -e.y, -e.z};
    v[2] = c + Vec3{e.x, e.y, -e.z};
    v[3] = c + Vec3{-e.x, e.y, -e.z};
    v[4] = c + Vec3{-e.x, -e.y, e.z};
    v[5] = c + Vec3{e.x, -e.y, e.z};
    v[6] = c + Vec3{e.x, e.y, e.z};
    v[7] = c + Vec3{-e.x, e.y, e.z};

    std::vector<u32> indices = {// front (z+)
                                4, 5, 6, 4, 6, 7,
                                // back (z-)
                                0, 2, 1, 0, 3, 2,
                                // left (x-)
                                0, 7, 3, 0, 4, 7,
                                // right (x+)
                                1, 2, 6, 1, 6, 5,
                                // top (y+)
                                3, 7, 6, 3, 6, 2,
                                // bottom (y-)
                                0, 1, 5, 0, 5, 4};
    draw_triangles(v, indices, color);
}

void DebugDrawer::clear() {
    this->line_vertices.clear();
    this->triangle_vertices.clear();
    this->triangle_indices.clear();
}

DebugDrawer::DebugDrawer() {
    instance = this;
    ResourceLoader *loader = ResourceLoader::get_instance();
    Ref<Shader> debug_shader =
        loader->load<Shader>("assets/shader/debug.slang");
    debug_desc.add_attr(0, VertexAttributeType::FLOAT, 3);
    debug_desc.add_attr(1, VertexAttributeType::UNSIGNED_BYTE, 4, true);
    RenderRasterizerState rst;
    rst.poly_mode = PolygonMode::FILL;
    RenderDepthStencilState depth;
    depth.depth_on = true;
    RenderBlendState blend;
    blend.func = BlendFunc::create(
        BlendFactor::SRC_COLOR, BlendFactor::ONE_MINUS_SRC_COLOR,
        BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA);
    blend.blend_on = true;
    debug_mat.create(debug_shader, rst, depth, blend);
}
}  // namespace Seed
