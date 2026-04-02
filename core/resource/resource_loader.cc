#include "resource_loader.h"
#include "core/io/file.h"
#include "core/io/dir.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "core/rendering/mesh.h"
#include "core/resource/model_file.h"
#include <filesystem>
#include <type_traits>
#include <nlohmann/json.hpp>
#include <vector>
#include "core/serialize/json_impl.h"

#include "core/resource/model.h"
#include "core/resource/terrain.h"
#include "core/resource/texture.h"
#include "core/resource/sky.h"
#include "core/resource/image.h"
#include "core/resource/billboard.h"
#include "mappable_texture.h"

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

void ResourceLoader::load_meshes(const std::string &path, std::vector<Ref<Mesh>> &meshes) {
    Ref<File> file = File::open(path, "rb");
    Ref<Dir> dir = Dir::open(file->get_directory());
    auto model_info = file->read_json();
    std::string bin_path = model_info["bin_file"];
    Ref<File> bin_file = dir->open_file(bin_path, "rb");

    std::vector<i32> mesh_mats;
    std::vector<Ref<BaseMaterial>> materials;
    std::vector<Ref<Texture>> textures;
    auto jmeshs = model_info["meshes"];
    for (auto &jmesh : jmeshs) {
        std::vector<u32> indices;
        if (jmesh["has_bone"]) {
            std::vector<SkeletonVertex> vertices;
            bin_file->read_vector(vertices, jmesh["vertex_count"]);
            bin_file->read_vector(indices, jmesh["index_count"]);
            meshes.push_back(Ref<Mesh>(&DS::get_instance()->skeleton_mesh_desc,
                                      vertices, indices,
                                      (AABB)jmesh["bounding_box"]));
        } else {
            std::vector<ModelVertex> vertices;
            bin_file->read_vector(vertices, jmesh["vertex_count"]);
            bin_file->read_vector(indices, jmesh["index_count"]);
            meshes.push_back(Ref<Mesh>(&DS::get_instance()->mesh_desc, vertices,
                                      indices, (AABB)jmesh["bounding_box"]));
        }

        mesh_mats.push_back(jmesh["material_id"]);
    }

    auto jtextures = model_info["textures"];
    for (auto &jtexture : jtextures) {
        Ref<Texture> tex = load<Texture>(dir->concat(jtexture));
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
    for (int i = 0; i < meshes.size(); i++) {
        i32 id = mesh_mats[i];
        if (id == -1) id = 0;
        meshes[i]->set_material(ref_cast<Material>(materials[id]));
    }
}

template <>
Ref<BasicModel> ResourceLoader::_load(const std::string &path) {
    Ref<BasicModel> model;
    std::vector<Ref<Mesh>> meshes;
    load_meshes(path, meshes);
    model.create(meshes);
    return model;
}

template <>
Ref<SkeletonModel> ResourceLoader::_load(const std::string &path) {
    Ref<SkeletonModel> model;
    Ref<File> file = File::open(path, "rb");
    Ref<Dir> dir = Dir::open(file->get_directory());
    auto model_info = file->read_json();
    std::string bin_path = model_info["bin_file"];
    Ref<File> bin_file = dir->open_file(bin_path, "rb");

    std::vector<Ref<Mesh>> meshs;
    std::vector<i32> mesh_mats;
    std::vector<Ref<BaseMaterial>> materials;
    std::vector<Ref<Texture>> textures;
    Ref<Skeleton> skeleton;
    skeleton.create();
    auto jmeshs = model_info["meshes"];
    for (auto &jmesh : jmeshs) {
        std::vector<u32> indices;
        if (jmesh["has_bone"]) {
            std::vector<SkeletonVertex> vertices;
            bin_file->read_vector(vertices, jmesh["vertex_count"]);
            bin_file->read_vector(indices, jmesh["index_count"]);
            meshs.push_back(Ref<Mesh>(&DS::get_instance()->skeleton_mesh_desc,
                                      vertices, indices,
                                      (AABB)jmesh["bounding_box"]));
        } else {
            std::vector<ModelVertex> vertices;
            bin_file->read_vector(vertices, jmesh["vertex_count"]);
            bin_file->read_vector(indices, jmesh["index_count"]);
            meshs.push_back(Ref<Mesh>(&DS::get_instance()->mesh_desc, vertices,
                                      indices, (AABB)jmesh["bounding_box"]));
        }

        mesh_mats.push_back(jmesh["material_id"]);
    }

    if (model_info.contains("bones")) {
        auto jbones = model_info["bones"];
        u64 bin_offset = jbones["bin_offset"];
        u32 bone_cnt = (u64)jbones["bin_size"] / sizeof(Mat4);
        skeleton->bones.resize(bone_cnt);
        bin_file->read_vector(skeleton->bones, bone_cnt);
        skeleton->bone_parents = jbones["parents"].get<std::vector<u16>>();
    }

    auto jtextures = model_info["textures"];
    for (auto &jtexture : jtextures) {
        Ref<Texture> tex = load<Texture>(dir->concat(jtexture));
        if (tex.is_valid()) {
            textures.push_back(tex);
        }
    }

    auto jmaterials = model_info["materials"];
    for (auto &jmaterial : jmaterials) {
        Ref<BaseMaterial> mat;
        mat.create(DS::get_instance()->skeleton_mesh_shader);
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

    model.create(meshs, skeleton);

    auto janimations = model_info["animations"];
    for (auto &janimation : janimations) {
        struct ClipInfo {
                u16 bone_id;
                u16 position_key_count;
                u16 rotation_key_count;
                u16 scaling_key_count;
        } clip_info;
        Ref<Animation> animation;
        animation.create();
        animation->name = janimation["name"];
        animation->duration = janimation["duration"];
        u32 clip_count = janimation["clip_count"];
        animation->clips.reserve(clip_count);
        for (u32 i = 0; i < clip_count; i++) {
            bin_file->read(&clip_info);
            animation->clips.push_back({});
            AnimationClip &clip = animation->clips.back();
            clip.bone_id = clip_info.bone_id;
            bin_file->read_vector(clip.position_keys,
                                  clip_info.position_key_count);
            bin_file->read_vector(clip.rotation_keys,
                                  clip_info.rotation_key_count);
            bin_file->read_vector(clip.scaling_keys,
                                  clip_info.scaling_key_count);
        }
        model->add_animation(animation);
    }

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
Ref<MappableTexture> ResourceLoader::_load(const std::string &path) {
    Ref<MappableTexture> texture;
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
    void *data = stbi_load(path.c_str(), &w, &h, &comp, 4);

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
    Ref<File> file = File::open(path, "rb");
    Ref<Dir> dir = Dir::open(file->get_directory());
    auto terrain_info = file->read_json();
    std::string name = terrain_info["name"];
    u32 width = terrain_info["width"];
    u32 height = terrain_info["height"];
    Ref<Image> height_map;
    Ref<Texture> splat_map, light_map;
    auto jheight_map = terrain_info["height_map"];
    height_map = load<Image>(dir->concat(jheight_map));

    auto jsplat_map = terrain_info["splat_map"];
    splat_map = load<Texture>(dir->concat(jsplat_map));
    if (terrain_info.contains("light_map")) {
        auto jlight_map = terrain_info["light_map"];
        light_map = load<Texture>(dir->concat(jlight_map));
    }
    terrain.create(height_map, light_map, splat_map);
    if (terrain_info.contains("tex1")) {
        auto jtex1 = terrain_info["tex1"];
        auto texture = load<Texture>(dir->concat(jtex1));
        terrain->get_material()->set_texture("tex1", texture);
        texture->update_sampler(SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                                .wrap_v = SamplerWrap::REPEAT});
    }

    if (terrain_info.contains("tex1_normal")) {
        auto jtex1 = terrain_info["tex1_normal"];
        auto texture = load<Texture>(dir->concat(jtex1));
        terrain->get_material()->set_texture("tex1_normal", texture);
        texture->update_sampler(SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                                .wrap_v = SamplerWrap::REPEAT});
    }
    return terrain;
}

template <>
Ref<Billboard> ResourceLoader::_load(const std::string &path) {
    Ref<Billboard> billboard;
    Ref<Texture> image = _load<Texture>(path);
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