#include "mesh.h"
#include <glad/glad.h>

namespace Seed {
Ref<Mesh> Mesh::create(std::vector<Vertex> &vertices,
                       std::vector<Texture> &textures) {
    Ref<Mesh> mesh;
    mesh.create();
    mesh->vertices = std::move(vertices);
    mesh->textures = std::move(textures);

    return mesh;
}
}  // namespace Seed