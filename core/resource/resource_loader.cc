#include "resource_loader.h"
#include "core/io/file.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "core/resource/model_file.h"
#include <filesystem>
#include <type_traits>
#include <nlohmann/json.hpp>

#include "core/resource/model.h"
#include "core/resource/terrain.h"
#include "core/resource/texture.h"
#include "core/resource/sky.h"
#include "core/resource/image.h"
#include "core/resource/billboard.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Seed {
ResourceLoader *ResourceLoader::get_instance() { return instance; }

ResourceLoader::ResourceLoader() {
    instance = this;
    spdlog::info("Initializing Resource loader");
}

ResourceLoader::~ResourceLoader() { instance = nullptr; }

template <>
Ref<Shader> ResourceLoader::_load(const std::string &path) {
    Ref<Shader> shader;
    Ref<File> file = File::open(path, "rb");
    std::string shader_code;
    shader_code = file->read_str();
    shader.create(path, shader_code);
    return shader;
}

template <>
Ref<Model> ResourceLoader::_load(const std::string &path) {
    Ref<Model> model;
    Ref<File> file = File::open(path, "rb");
    std::vector<Ref<Mesh>> meshs;
    std::vector<i32> mesh_mats;
    std::vector<Ref<BaseMaterial>> materials;
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
        std::vector<ModelVertex> vertices;
        std::vector<u32> indices;
        MeshHeader mesh_header;
        file->read(&mesh_header);
        file->read_vector(vertices, mesh_header.vertex_size);
        file->read_vector(indices, mesh_header.index_size);
        meshs.push_back(Ref<Mesh>(&DS::get_instance()->mesh_desc, vertices,
                                  indices, mesh_header.bounding_box));
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
        Ref<BaseMaterial> mat;
        mat.create();
        file->read(&mat_field);
        mat->set_texture_map(BaseMaterial::DIFFUSE,
                             texture_map[mat_field.diffuse_map]);
        mat->set_texture_map(BaseMaterial::SPECULAR,
                             texture_map[mat_field.specular_map]);
        mat->set_texture_map(BaseMaterial::NORMAl,
                             texture_map[mat_field.normal_map]);
        RenderBlendState blend_state;
        blend_state.blend_on = mat_field.opacity != 1.0;
        materials.push_back(mat);
    }
    for (int i = 0; i < meshs.size(); i++) {
        i32 id = mesh_mats[i];
        if (id == -1) id = 2;
        meshs[i]->set_material(ref_cast<Material>(materials[id]));
    }
    model.create(meshs);
    return model;
}

template <>
Ref<Sky> ResourceLoader::_load(const std::string &path) {
    Ref<Sky> sky;
    Ref<File> json_file = File::open(path);
    if (json_file.is_null()) {
        SPDLOG_ERROR("Can't open json from {}", path);
        return sky;
    }
    nlohmann::json j = json_file->read_json();
    std::vector<u8 *> texture;
    texture.resize(6);
    int w, h, comp;
    for (auto tex_field : j) {
        u32 face = tex_field["face"];
        std::string tex_path = tex_field["path"];
        std::string r_tex_path =
            std::filesystem::path(path).parent_path().append(tex_path).string();
        u8 *data = stbi_load(r_tex_path.c_str(), &w, &h, &comp, 4);
        if (!data) {
            spdlog::warn("Can't load texture from {}", r_tex_path);
            return sky;
        }
        texture[face] = data;
    }
    sky.create(w, h, texture[0], texture[1], texture[2], texture[3], texture[4],
               texture[5]);

    return sky;
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
    texture.create(TextureType::TEXTURE_2D, w, h, PixelFormat::RGBA,
                   (const u8 *)data);

    stbi_image_free(data);
    return texture;
}

template <>
Ref<Image> ResourceLoader::_load(const std::string &path) {
    Ref<Image> image;
    int w, h, comp;
    stbi_set_flip_vertically_on_load(true);
    void *data = stbi_load(path.c_str(), &w, &h, &comp, 4);
    stbi_set_flip_vertically_on_load(false);

    if (!data) {
        spdlog::warn("Can't load image from {}", path);
        return image;
    }
    image.create(PixelFormat::RGBA, w, h);
    image->update((u8 *)data, w, h);
    stbi_image_free(data);
    return image;
}

template <>
Ref<Terrain> ResourceLoader::_load(const std::string &path) {
    Ref<Terrain> terrain;
    Ref<Image> height_map = _load<Image>(path);
    terrain.create(height_map);
    return terrain;
}

template <>
Ref<Billboard> ResourceLoader::_load(const std::string &path) {
    Ref<Billboard> billboard;
    Ref<Image> image = _load<Image>(path);
    billboard.create(image);
    return billboard;
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