#ifndef _SEED_MESH_H_
#define _SEED_MESH_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/ref.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/material.h"
#include <vector>
#include "core/rendering/vertex_data.h"

namespace Seed {
struct Vertex {
        Vec3 position;
        Vec3 normal;
        Vec2 tex_coord;
};

struct Mesh : RefCounted {
        VertexData vertex_data;
        Ref<Material> material;

        Mesh(const std::vector<Vertex> &vertices,
             const std::vector<u32> &indices)
            : vertex_data(sizeof(Vertex), vertices.size(), vertices.data(),
                          indices) {}

        Mesh(const std::vector<Vertex> &vertices,
             const std::vector<u32> &indices, Ref<Material> material)
            : Mesh(vertices, indices) {
            this->material = material;
        }

        void set_material(Ref<Material> mat) { this->material = mat; }
        Ref<Material> get_material() { return material; }
};

}  // namespace Seed

#endif