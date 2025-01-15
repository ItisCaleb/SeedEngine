#include "mesh.h"
#include <glad/glad.h>

namespace Seed {
Ref<Mesh> Mesh::create(std::vector<Vertex> &vertices, std::vector<u32> &indices,
                       std::vector<Texture> &textures) {
    Ref<Mesh> mesh;
    mesh.create();
    mesh->vertices = std::move(vertices);
    mesh->indices = std::move(indices);
    mesh->textures = std::move(textures);

    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->EBO);

    glBindVertexArray(mesh->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh->vertices.size(),
                 mesh->vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * mesh->indices.size(),
                 mesh->indices.data(), GL_STATIC_DRAW);
    /* position */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    /* normal */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)(3 * sizeof(f32)));
    /* tex coords*/
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)(6 * sizeof(f32)));
    glEnableVertexAttribArray(0);
    return mesh;
}
}  // namespace Seed