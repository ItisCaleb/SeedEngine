#ifndef _SEED_DEBUG_DRAWER_H_
#define _SEED_DEBUG_DRAWER_H_
#include "core/math/vec3.h"
#include "core/rendering/render_common.h"
#include "core/collision/shape.h"
#include "core/resource/material.h"
#include <vector>
#include <mutex>

namespace Seed {
class DefaultRenderer;
class DebugDrawer {
        friend DefaultRenderer;

    private:
        struct DebugVertex {
                Vec3 p;
                Color color;
        };
        std::mutex mu;
        std::vector<DebugVertex> line_vertices;
        std::vector<DebugVertex> triangle_vertices;
        std::vector<u32> triangle_indices;
        Ref<Material> debug_mat;
        VertexLayout debug_desc;

    public:
        void draw_line(Vec3 from, Vec3 to, Color color = Color{255, 0, 0, 64});
        void draw_triangle(Vec3 v1, Vec3 v2, Vec3 v3,
                           Color color = Color{255, 0, 0, 64});
        void draw_triangles(const std::vector<Vec3> vertices,
                            const std::vector<u32> indices,
                            Color color = Color{255, 0, 0, 64});
        void draw_frustum(const Frustum &frustum,
                          Color color = Color{255, 0, 0, 64});
        void draw_aabb(const AABB &aabb, Color color = Color{0, 255, 0, 64});
        void clear();
        VertexLayout *get_debug_desc() { return &debug_desc; }
        bool try_lock() { return mu.try_lock(); };
        void lock() { mu.lock(); }
        void unlock() { mu.unlock(); }
        DebugDrawer();
};
}  // namespace Seed

#endif