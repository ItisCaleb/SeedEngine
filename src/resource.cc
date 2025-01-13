#include <glad/glad.h>
#include <seed/resource.h>
#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure
#include <thirdparty/stb_image.h>

#include <assimp/Importer.hpp>  // C++ importer interface

namespace Seed {
ResourceLoader *ResourceLoader::get_instance() { return instance; }

ResourceLoader::ResourceLoader() {
    instance = this;
    spdlog::info("Initializing Resource loader");
}

ResourceLoader::~ResourceLoader() { instance = nullptr; }

template <>
Ref<Texture> ResourceLoader::load(std::string path) {
    int w, h, comp;
    Texture *tex = new Texture;
    stbi_set_flip_vertically_on_load(true);
    void *data = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!data) return Ref<Texture>();

    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data);

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Ref(tex);
}

template <>
Ref<Mesh> ResourceLoader::load(std::string path) {
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(
        path, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                  aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    // If the import failed, report it
    if (!scene) {
        spdlog::error("Can't load Mesh from {}", path);
        return Ref<Mesh>();
    }

    Mesh *mesh = new Mesh;
    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->EBO);

    glBindVertexArray(mesh->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh->vertices.size(),
                 mesh->vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(u32) * mesh->indices.size(),
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
    return Ref<Mesh>(mesh);
}

}  // namespace Seed