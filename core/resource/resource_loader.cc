#include <glad/glad.h>
#include "resource_loader.h"
#include "core/io/file.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "core/rendering/model_file.h"
#include <filesystem>
#include <type_traits>

#include "core/resource/model.h"
#include "core/resource/terrain.h"
#include "core/resource/texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Seed {
ResourceLoader *ResourceLoader::get_instance() { return instance; }

ResourceLoader::ResourceLoader() {
    instance = this;
    spdlog::info("Initializing Resource loader");
}

ResourceLoader::~ResourceLoader() { instance = nullptr; }

Ref<Shader> ResourceLoader::load_shader(const std::string &vertex_path,
                                        const std::string &fragment_path,
                                        const std::string &geometry_path,
                                        const std::string &tess_ctrl_path,
                                        const std::string &tess_eval_path) {
    Ref<Shader> shader;
    std::string vertex_s, fragment_s, geometry_s, tess_ctrl_s, tess_eval_s;

    Ref<File> vertex_f = File::open(vertex_path, "r");
    if (vertex_f.is_null()) {
        throw std::runtime_error("Can't open vertex shader.");
    }
    Ref<File> fragment_f = File::open(fragment_path, "r");
    if (fragment_f.is_null()) {
        throw std::runtime_error("Can't open fragment shader.");
    }
    if (!geometry_path.empty()) {
        Ref<File> geomertry_f = File::open(geometry_path, "r");
        if (geomertry_f.is_null()) {
            throw std::runtime_error("Can't open geometry shader.");
        }
        geometry_s = geomertry_f->read_str();
    }

    if (!tess_ctrl_path.empty()) {
        Ref<File> tess_ctrl_f = File::open(tess_ctrl_path, "r");
        if (tess_ctrl_f.is_null()) {
            throw std::runtime_error("Can't open tesselation control shader.");
        }
        tess_ctrl_s = tess_ctrl_f->read_str();
    }
    if (!tess_eval_path.empty()) {
        Ref<File> tess_eval_f = File::open(tess_eval_path, "r");
        if (tess_eval_f.is_null()) {
            throw std::runtime_error(
                "Can't open tesselation evaluation shader.");
        }
        tess_eval_s = tess_eval_f->read_str();
    }
    vertex_s = vertex_f->read_str();
    fragment_s = fragment_f->read_str();
    shader.create(vertex_s, fragment_s, geometry_s, tess_ctrl_s,
        tess_eval_s);
    return shader;
}

template <>
Ref<Model> ResourceLoader::_load(const std::string &path) {
    Ref<Model> model;
    Ref<File> file = File::open(path, "rb");
    std::vector<Ref<Mesh>> meshs;
    std::vector<u32> mesh_mats;
    std::vector<Ref<Material>> materials;
    std::map<i32, Ref<Texture>> texture_map;
    std::string magic = file->read_str(strlen(model_file_magic));
    if (memcmp(magic.c_str(), model_file_magic, strlen(model_file_magic)) !=
        0) {
        spdlog::warn("Can't load model file '{}'", path);
        return model;
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
        meshs.push_back(Ref<Mesh>(vertices, indices));
        mesh_mats.push_back(mesh_header.material_id);
    }

    std::filesystem::path dir = path;
    std::string directory = dir.parent_path().string();
    for (int i = 0; i < model_header.texture_count; i++) {
        TextureField tex_field;
        file->read(&tex_field);
        std::string tex_path = file->read_str(tex_field.path_length);
        Ref<Texture> tex =
            load<Texture>(fmt::format("{}/{}", directory, tex_path));
        if (tex.is_valid()) {
            texture_map[i] = tex;
        }
    }
    for (int i = 0; i < model_header.material_count; i++) {
        MaterialField mat_field;
        Ref<Material> mat;
        mat.create();
        file->read(&mat_field);
        mat->set_texture_map(Material::DIFFUSE,
                             texture_map[mat_field.diffuse_map]);
        mat->set_texture_map(Material::SPECULAR,
                             texture_map[mat_field.specular_map]);
        mat->set_texture_map(Material::NORMAl,
                             texture_map[mat_field.normal_map]);
        materials.push_back(mat);
    }
    for (int i = 0; i < meshs.size(); i++) {
        meshs[i]->set_material(materials[mesh_mats[i]]);
    }
    model.create(meshs, model_header.bounding_box);
    return model;
}

template <>
Ref<Texture> ResourceLoader::_load(const std::string &path) {
    Ref<Texture> texture;
    int w, h, comp;

    void *data = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!data) {
        spdlog::warn("Can't load texture from {}", path);
        return texture;
    }
    texture.create(w, h, (const u8 *)data);

    stbi_image_free(data);
    return texture;
}

template <>
Ref<Terrain> ResourceLoader::_load(const std::string &path) {
    Ref<Terrain> terrain;
    Ref<Texture> height_map = _load<Texture>(path);
    terrain.create(height_map->get_width(), height_map->get_height(),
                   height_map);
    return terrain;
}

void ResourceLoader::register_resource(Resource *res) {
    if (res == nullptr) return;
    this->res_cache[res->get_path()] = res;
}

void ResourceLoader::unregister_resource(Resource *res) {
    if (res == nullptr) return;
    this->res_cache.erase(res->get_path());
}

}  // namespace Seed