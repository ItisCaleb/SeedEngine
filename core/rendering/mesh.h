#ifndef _SEED_MESH_H_
#define _SEED_MESH_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/ref.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/material.h"
#include <vector>

namespace Seed {
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 tex_coord;
};

struct Mesh {
    RenderResource vertices_rc;
    RenderResource indices_rc;

    Ref<Material> material;

    Mesh(std::vector<Vertex> &vertices, std::vector<u32> &indices) {
        this->vertices_rc.alloc_vertex(sizeof(Vertex), vertices.size(),
                                       vertices.data());
        this->indices_rc.alloc_index(indices);
    }

    Mesh(std::vector<Vertex> &vertices, std::vector<u32> &indices, Ref<Material> material):Mesh(vertices, indices) {
        this->material = material;
    }

    void set_material(Ref<Material> mat){
        this->material = mat;
    }
    Ref<Material> get_material(){
        return material;
    }
};

}  // namespace Seed

#endif