#ifndef _SEED_MESH_H_
#define _SEED_MESH_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/ref.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/material.h"
#include "core/rendering/vertex_data.h"
#include "core/collision/aabb.h"
#include <vector>
#include <algorithm>

namespace Seed {
struct Vertex {
        Vec3 position;
        Vec3 normal;
        Vec2 tex_coord;
};

class Mesh : public RefCounted {
    private:
        static AABB calculate_aabb(const std::vector<Vertex> &vertices) {
            f32 x1 = 1e5, x2 = -1e5;
            f32 y1 = 1e5, y2 = -1e5;
            f32 z1 = 1e5, z2 = -1e5;
            for (auto &vertex : vertices) {
                x1 = std::min(x1, vertex.position.x);
                x2 = std::max(x2, vertex.position.x);
                y1 = std::min(y1, vertex.position.y);
                y2 = std::max(y2, vertex.position.y);
                z1 = std::min(z1, vertex.position.z);
                z2 = std::max(z2, vertex.position.z);
            }

            f32 w = (x2 - x1) / 2;
            f32 h = (y2 - y1) / 2;
            f32 d = (z2 - z1) / 2;
            return AABB{Vec3{x2 - w, y2 - h, z2 - d}, Vec3{w, h, d}};
        }

    public:
        VertexData vertex_data;
        Ref<Material> material;
        AABB bounding_box;
        RenderResource instance_idx_rc;

        Mesh(const std::vector<Vertex> &vertices,
             const std::vector<u32> &indices)
            : Mesh(vertices, indices, Ref<Material>()) {}

        Mesh(const std::vector<Vertex> &vertices,
             const std::vector<u32> &indices, Ref<Material> material)
            : Mesh(vertices, indices, material, calculate_aabb(vertices)) {}

        Mesh(const std::vector<Vertex> &vertices,
             const std::vector<u32> &indices, const AABB &bounding_box)
            : Mesh(vertices, indices, Ref<Material>(), bounding_box) {}

        Mesh(const std::vector<Vertex> &vertices,
             const std::vector<u32> &indices, Ref<Material> material,
             const AABB &bounding_box)
            : vertex_data(sizeof(Vertex), vertices.size(), vertices.data(),
                          indices),
              material(material),
              bounding_box(bounding_box) {
            instance_idx_rc.alloc_vertex(sizeof(u32), 0, nullptr);
        }

        ~Mesh() { instance_idx_rc.dealloc(); }

        void set_material(Ref<Material> mat) { this->material = mat; }
        Ref<Material> get_material() { return material; }
        const AABB &get_bounding_box() { return bounding_box; }
};

}  // namespace Seed

#endif