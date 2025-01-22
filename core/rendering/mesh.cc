#include "mesh.h"
#include <glad/glad.h>
#include "core/math/mat4.h"
#include "core/rendering/api/render_engine.h"

namespace Seed {

Mesh::~Mesh() {
    this->indices_rc.dealloc();
    this->vertices_rc.dealloc();
    RenderEngine::get_instance()->remove_mesh(this);
}

void Mesh::create(std::vector<Vertex> &vertices, std::vector<u32> &indices) {

    this->vertices_rc.alloc_vertex(sizeof(Vertex), vertices.size(),
                                   vertices.data());
    this->indices_rc.alloc_index(indices);

    RenderEngine::get_instance()->register_mesh(this);
}
}  // namespace Seed