#ifndef _SEED_MESH_H_
#define _SEED_MESH_H_
#include "core/ref.h"
#include "core/resource/material.h"
#include "core/rendering/vertex_data.h"
#include "core/collision/shape.h"
#include <vector>

#define LOD_MAX 4

namespace Seed {

class Mesh : public RefCounted {
    private:
        AABB bounding_box;
        RenderPrimitiveType type = RenderPrimitiveType::TRIANGLES;
        std::vector<u32> gen_indices(u32 length) {
            std::vector<u32> indices;
            for (u32 i = 0; i < length; i++) {
                indices.push_back(i);
            }
            return std::move(indices);
        }

    public:
        Ref<VertexData> vertex_data;
        std::vector<Ref<IndexData>> lod_indices;
        Ref<Material> material;

        Mesh(Ref<VertexData> vertex_data, Ref<Material> material,
             const AABB &bounding_box)
            : vertex_data(vertex_data),
              material(material),
              bounding_box(bounding_box) {
            this->lod_indices.reserve(LOD_MAX);
            this->lod_indices.emplace_back(
                gen_indices(vertex_data->get_count()),
                UpdateFrequence::STATIC);
        }

        template <typename T>
        Mesh(VertexLayout *layout, const std::vector<T> &vertices,
             Ref<Material> material, const AABB &bounding_box)
            : Mesh(layout, vertices, gen_indices(vertices.size()), material,
                   bounding_box) {}

        template <typename T>
        Mesh(VertexLayout *layout, const std::vector<T> &vertices,
             const std::vector<u32> &indices, const AABB &bounding_box)
            : Mesh(layout, vertices, indices, Ref<Material>(), bounding_box) {}

        template <typename T>
        Mesh(VertexLayout *layout, const std::vector<T> &vertices,
             const std::vector<u32> &indices, Ref<Material> material,
             const AABB &bounding_box)
            : material(material), bounding_box(bounding_box) {
            vertex_data.create(layout, vertices, UpdateFrequence::STATIC);
            this->lod_indices.reserve(LOD_MAX);
            this->lod_indices.emplace_back(indices, UpdateFrequence::STATIC);
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