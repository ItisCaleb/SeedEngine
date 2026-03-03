#include "resource_loader.h"
#include "core/io/file.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "core/resource/model_file.h"
#include <filesystem>
#include <type_traits>
#include <nlohmann/json.hpp>
#include "core/serialize/json_impl.h"

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
    auto model_info = file->read_json();
    std::string bin_path = model_info["bin_file"];
    Ref<File> bin_file = File::open(bin_path, "rb");

    std::vector<Ref<Mesh>> meshs;
    std::vector<i32> mesh_mats;
    std::vector<Ref<BaseMaterial>> materials;
    std::vector<Ref<Texture>> textures;
    std::string magic = file->read_str(strlen(model_file_magic));
    auto jmeshs = model_info["meshes"];
    for (auto &jmesh : jmeshs) {
        std::vector<u32> indices;
        if (jmesh["has_bone"]) {
            std::vector<SkeletonVertex> vertices;
            bin_file->read_vector(vertices, jmesh["vertex_count"]);
            bin_file->read_vector(indices, jmesh["index_count"]);
            meshs.push_back(Ref<Mesh>(&DS::get_instance()->mesh_desc, vertices,
                                      indices, (AABB)jmesh["bounding_box"]));
        } else {
            std::vector<ModelVertex> vertices;
            bin_file->read_vector(vertices, jmesh["vertex_count"]);
            bin_file->read_vector(indices, jmesh["index_count"]);
            meshs.push_back(Ref<Mesh>(&DS::get_instance()->mesh_desc, vertices,
                                      indices, (AABB)jmesh["bounding_box"]));
        }

        mesh_mats.push_back(jmesh["material_id"]);
    }

    std::filesystem::path dir = path;
    std::string directory = dir.parent_path().string();
    auto jtextures = model_info["textures"];
    for (auto &jtexture : jtextures) {
        Ref<Texture> tex = load<Texture>(
            fmt::format("{}/{}", directory, (std::string)jtexture));
        if (tex.is_valid()) {
            textures.push_back(tex);
        }
    }

    auto jmaterials = model_info["materials"];
    for (auto &jmaterial : jmaterials) {
        Ref<BaseMaterial> mat;
        mat.create();
        if (jmaterial["diffuse"] != -1) {
            mat->set_texture_map(BaseMaterial::DIFFUSE,
                                 textures[jmaterial["diffuse"]]);
        }
        if (jmaterial["specular"] != -1) {
            mat->set_texture_map(BaseMaterial::SPECULAR,
                                 textures[jmaterial["specular"]]);
        }
        if (jmaterial["normal"] != -1) {
            mat->set_texture_map(BaseMaterial::NORMAl,
                                 textures[jmaterial["normal"]]);
        }
        RenderBlendState blend_state;
        blend_state.blend_on = jmaterial["opacity"] != 1.0;
        materials.push_back(mat);
    }
    for (int i = 0; i < meshs.size(); i++) {
        i32 id = mesh_mats[i];
        if (id == -1) id = 0;
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
    stbi_image_free(texture[0]);
    stbi_image_free(texture[1]);
    stbi_image_free(texture[2]);
    stbi_image_free(texture[3]);
    stbi_image_free(texture[4]);
    stbi_image_free(texture[5]);
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