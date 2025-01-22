#include <glad/glad.h>
#include "resource.h"
#include "io/file.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "rendering/model_file.h"
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Seed {
ResourceLoader *ResourceLoader::get_instance() { return instance; }

ResourceLoader::ResourceLoader() {
    instance = this;
    spdlog::info("Initializing Resource loader");
}

ResourceLoader::~ResourceLoader() { instance = nullptr; }

RenderResource ResourceLoader::load_texture(const std::string &path) {
    int w, h, comp;
    RenderResource tex_rc;
    stbi_set_flip_vertically_on_load(true);
    void *data = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!data) {
        spdlog::warn("Can't load texture from {}", path);
        return tex_rc;
    }
    tex_rc.alloc_texture(w, h, data);

    stbi_image_free(data);
    return tex_rc;
}

RenderResource ResourceLoader::loadShader(const std::string &vertex_path,
                                          const std::string &fragment_path) {
    RenderResource shader_rc;
    Ref<File> vertex_f = File::open(vertex_path, "r");
    if (vertex_f.is_null()) {
        throw std::runtime_error("Can't open vertex shader.");
    }
    Ref<File> fragment_f = File::open(fragment_path, "r");
    if (fragment_f.is_null()) {
        throw std::runtime_error("Can't open fragment shader.");
    }
    std::string vertex_s = vertex_f->read_str();
    std::string fragment_s = fragment_f->read_str();
    const char *vertex_code = vertex_s.c_str();
    const char *fragment_code = fragment_s.c_str();
    try {
        shader_rc.alloc_shader(vertex_code, fragment_code);
        return shader_rc;
    } catch (std::exception &e) {
        throw e;
    }
}

template <>
Ref<Model> ResourceLoader::load(const std::string &path) {
    Ref<File> file = File::open(path, "rb");
    std::vector<Mesh> meshs;
    std::vector<Ref<Material>> materials;
    std::map<i32, RenderResource> texture_map;
    std::string magic = file->read_str(strlen(model_file_magic));
    if (memcmp(magic.c_str(), model_file_magic, strlen(model_file_magic)) !=
        0) {
        spdlog::warn("Can't load model file '{}'", path);
        return {};
    }
    ModelHeader model_header;
    file->read(&model_header);
    for (int i = 0; i < model_header.mesh_count; i++) {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        MeshHeader mesh_header;
        file->read(&mesh_header);
        file->read_vector(vertices, mesh_header.vertex_size);
        file->read_vector(indices, mesh_header.index_size);
        meshs.push_back(Mesh(vertices, indices, mesh_header.material_id));
    }

    std::filesystem::path dir = path;
    std::string directory = dir.parent_path().string();
    for (int i = 0; i < model_header.texture_count; i++) {
        TextureField tex_field;
        file->read(&tex_field);
        std::string tex_path = file->read_str(tex_field.path_length);
        RenderResource rc =
            load_texture(fmt::format("{}/{}", directory, tex_path));
        if (rc.inited()) {
            texture_map[i] = rc;
        }
    }
    for (int i = 0; i < model_header.material_count; i++) {
        MaterialField mat_field;
        file->read(&mat_field);
        RenderResource diffuse_rc = texture_map[mat_field.diffuse_map];
        RenderResource specular_rc = texture_map[mat_field.specular_map];
        RenderResource normal_rc = texture_map[mat_field.normal_map];
        materials.push_back(
            Material::create(diffuse_rc, specular_rc, normal_rc));
    }
    return Model::create(meshs, materials);
}

}  // namespace Seed