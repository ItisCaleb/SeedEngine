#include "mesh.h"
#include <glad/glad.h>
#include "core/math/mat4.h"
#include "core/rendering/api/render_engine.h"

namespace Seed {

Mesh::~Mesh() {
    this->indices_rc.dealloc();
    this->vertices_rc.dealloc();
    this->instance_rc.dealloc();
    RenderEngine::get_instance()->remove_mesh(this);
}

Ref<Mesh> Mesh::create(std::vector<Vertex> &vertices, std::vector<u32> &indices) {
    Ref<Mesh> mesh;
    mesh.create();
    std::vector<VertexAttribute> vertices_attrs = {
        {.layout_num = 0, .size = 3, .stride = sizeof(Vertex)},
        {.layout_num = 1, .size = 3, .stride = sizeof(Vertex)},
        {.layout_num = 2, .size = 2, .stride = sizeof(Vertex)}};
    std::vector<VertexAttribute> instance_attrs = {
        {
            .layout_num = 3,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 4,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 5,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 6,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
    };
    mesh->vertices_rc.alloc_vertex(sizeof(Vertex), vertices.size(),
                                   vertices.data());
    mesh->indices_rc.alloc_index(indices);
    mesh->vertices_desc_rc.alloc_vertex_desc(vertices_attrs);
    mesh->instance_rc.alloc_vertex(sizeof(Mat4), 1, NULL);
    mesh->instance_desc_rc.alloc_vertex_desc(instance_attrs);
    RenderEngine::get_instance()->register_mesh(*mesh);
    return mesh;
}
}  // namespace Seed