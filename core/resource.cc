#include <glad/glad.h>
#include "resource.h"
#include "io/file.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure
#include <stb_image.h>

#include <assimp/Importer.hpp>  // C++ importer interface

namespace Seed {
ResourceLoader *ResourceLoader::get_instance() { return instance; }

ResourceLoader::ResourceLoader() {
    instance = this;
    spdlog::info("Initializing Resource loader");
}

ResourceLoader::~ResourceLoader() { instance = nullptr; }

template <>
Ref<Texture> ResourceLoader::load(const std::string &path) {
    int w, h, comp;
    Ref<Texture> tex;
    stbi_set_flip_vertically_on_load(true);
    void *data = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!data) return tex;
    tex.create();
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
    return tex;
}

template <>
Ref<Mesh> ResourceLoader::load(const std::string &path) {
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(
        path, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                  aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    Ref<Mesh> mesh;
    // If the import failed, report it
    if (!scene) {
        spdlog::error("Can't load Mesh from {}", path);
        return mesh;
    }
    
    mesh.create();
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

Ref<Shader> ResourceLoader::loadShader(const std::string &vertex_path,
                                  const std::string &fragment_path) {
    Ref<File> vertex_f = File::open(vertex_path, "r");
    if (vertex_f.is_null()) {
        throw std::exception("Can't open vertex shader.");
    }
    Ref<File> fragment_f = File::open(fragment_path, "r");
    if (fragment_f.is_null()) {
        throw std::exception("Can't open fragment shader.");
    }
    std::string vertex_s = vertex_f->read();
    std::string fragment_s = fragment_f->read();
    const char *vertex_code = vertex_s.c_str();
    const char *fragment_code = fragment_s.c_str();

    u32 vertex, fragment;
    int success;
    char info[512];
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_code, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, info);
        throw std::exception(info);
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_code, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, info);
        throw std::exception(info);
    }
    u32 id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, info);
        throw std::exception(info);
    }
    Ref<Shader> shader(id);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return shader;
}

}  // namespace Seed