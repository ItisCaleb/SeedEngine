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

#define LOD_MAX 4

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
        AABB bounding_box;
        RenderPrimitiveType type = RenderPrimitiveType::TRIANGLES;

    public:
        Ref<VertexData> vertex_data;
        std::vector<Ref<IndexData>> lod_indices;
        Ref<Material> material;

        template <typename T>
        Mesh(VertexLayout *layout, const std::vector<T> &vertices,
             const std::vector<u32> &indices, const AABB &bounding_box)
            : Mesh(layout, vertices, indices, Ref<Material>(), bounding_box) {
        }

        template <typename T>
        Mesh(VertexLayout *layout, const std::vector<T> &vertices,
             const std::vector<u32> &indices, Ref<Material> material,
             const AABB &bounding_box)
            : material(material), bounding_box(bounding_box) {
            vertex_data.create(layout, vertices);
            this->lod_indices.reserve(LOD_MAX);
            this->lod_indices.emplace_back(indices);
        }

        ~Mesh() {}

        void set_material(Ref<Material> mat) { this->material = mat; }
        Ref<Material> get_material() { return material; }
        void set_type(RenderPrimitiveType prim_type) { this->type = prim_type; }
        RenderPrimitiveType get_type() { return this->type; }
        const AABB &get_bounding_box() { return bounding_box; }
        void set_bounding_box(const AABB &aabb) { this->bounding_box = aabb; }
};

}  // namespace Seed

#endif