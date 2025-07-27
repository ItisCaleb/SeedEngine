#ifndef _SEED_DEBUG_DRAWER_H_
#define _SEED_DEBUG_DRAWER_H_
#include "core/math/vec3.h"
#include "core/rendering/render_common.h"
#include "core/resource/material.h"
#include <vector>

namespace Seed {
class DefaultRenderer;
class DebugDrawer {
        friend DefaultRenderer;

    private:
        struct DebugVertex {
                Vec3 p;
                Color color;
        };
        inline static DebugDrawer *instance = nullptr;
        std::vector<DebugVertex> line_vertices;
        std::vector<DebugVertex> triangle_vertices;
        std::vector<u32> triangle_indices;
        Ref<Material> debug_mat;
        VertexLayout debug_desc;

    public:
        void draw_line(Vec3 from, Vec3 to, Color color);
        void draw_triangle(Vec3 v1, Vec3 v2, Vec3 v3, Color color);
        VertexLayout *get_debug_desc(){
            return &debug_desc;
        }
        static DebugDrawer *get_instance() { return instance; }
        DebugDrawer();
};
}  // namespace Seed

#endif