#include "resource_loader.h"
#include "core/container/kstring.h"
#include "core/engine.h"
#include "core/io/file.h"
#include "core/io/dir.h"
#include <spdlog/spdlog.h>
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/ref.h"
#include "core/rendering/mesh.h"

#include <nlohmann/json.hpp>
#include <vector>
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource_entry.h"
#include "core/serialize/json_impl.h"

#include "core/resource/model.h"
#include "core/resource/terrain.h"
#include "core/resource/texture.h"
#include "core/resource/image.h"
#include "core/types.h"
#include "resource.h"
#include "shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Seed {
ResourceLoader *ResourceLoader::get_instance() { return instance; }

ResourceLoader::~ResourceLoader() { instance = nullptr; }

Ref<Resource> ResourceLoader::load_shader(ResourceLoader &loader,
                                          ResourceConfiguration &config,
                                          Ref<File> data) {
    Ref<Shader> shader;
    std::string shader_code;
    shader_code = data->read_str();
    shader.create(data->get_fullpath(), shader_code);
    return ref_cast<Resource>(shader);
}

static void load_meshes(ResourceLoader &loader, ResourceConfiguration &config,
                        Ref<File> data, std::vector<Ref<Mesh>> &meshes) {
    auto model_info = config.get_json();

    std::vector<i32> mesh_mats;
    std::vector<Ref<BaseMaterial>> materials;
    std::vector<Ref<Texture>> textures;
    auto jmeshs = model_info["meshes"];
    for (auto &jmesh : jmeshs) {
        std::vector<u32> indices;
        if (jmesh["has_bone"]) {
            std::vector<SkeletonVertex> vertices;
            data->read_vector(vertices, jmesh["vertex_count"]);
            data->read_vector(indices, jmesh["index_count"]);
            meshes.push_back(Ref<Mesh>(&DS::get_instance()->skeleton_mesh_desc,
                                       vertices, indices,
                                       (AABB)jmesh["bounding_box"]));
        } else {
            std::vector<ModelVertex> vertices;
            data->read_vector(vertices, jmesh["vertex_count"]);
            data->read_vector(indices, jmesh["index_count"]);
            meshes.push_back(Ref<Mesh>(&DS::get_instance()->mesh_desc, vertices,
                                       indices, (AABB)jmesh["bounding_box"]));
        }

        mesh_mats.push_back(jmesh["material_id"]);
    }

    auto jmaterials = model_info["materials"];
    for (auto &jmaterial : jmaterials) {
        Ref<BaseMaterial> mat;
        mat.create();
        UUID diffuse = jmaterial["diffuse"];
        UUID specular = jmaterial["specular"];
        UUID normal = jmaterial["normal"];
        if (!diffuse.is_null()) {
            mat->set_texture_map(BaseMaterial::DIFFUSE,
                                 loader.load<Texture>(diffuse));
        }
        if (!specular.is_null()) {
            mat->set_texture_map(BaseMaterial::SPECULAR,
                                 loader.load<Texture>(specular));
        }
        if (!normal.is_null()) {
            mat->set_texture_map(BaseMaterial::NORMAl,
                                 loader.load<Texture>(normal));
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

Ref<Resource> ResourceLoader::load_basic_model(ResourceLoader &loader,
                                               ResourceConfiguration &config,
                                               Ref<File> data) {
    Ref<BasicModel> model;
    std::vector<Ref<Mesh>> meshes;
    load_meshes(loader, config, data, meshes);
    model.create(meshes);
    return ref_cast<Resource>(model);
}

Ref<Resource> ResourceLoader::load_skeleton_model(ResourceLoader &loader,
                                                  ResourceConfiguration &config,
                                                  Ref<File> data) {
    Ref<SkeletonModel> model;
    Ref<Dir> dir = Dir::open(data->get_directory());
    auto model_info = config.get_json();
    Ref<File> bin_file = data;

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

    auto jmaterials = model_info["materials"];
    for (auto &jmaterial : jmaterials) {
        Ref<BaseMaterial> mat;
        mat.create(DS::get_instance()->skeleton_mesh_shader);
        UUID diffuse = jmaterial["diffuse"];
        UUID specular = jmaterial["specular"];
        UUID normal = jmaterial["normal"];
        if (!diffuse.is_null()) {
            mat->set_texture_map(BaseMaterial::DIFFUSE,
                                 loader.load<Texture>(diffuse));
        }
        if (!specular.is_null()) {
            mat->set_texture_map(BaseMaterial::SPECULAR,
                                 loader.load<Texture>(specular));
        }
        if (!normal.is_null()) {
            mat->set_texture_map(BaseMaterial::NORMAl,
                                 loader.load<Texture>(normal));
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
    return ref_cast<Resource>(model);
}
/* since we now use malloc in update heap, we do not need to free here */
RHI::UpdateBufferInfo ResourceLoader::load_image_to_upload(UUID uuid,
                                                           bool force_rgba) {
    ResourceEntry *entry = entries.get_entry(uuid);
    RHI::UpdateBufferInfo info;
    info.data = nullptr;
    if (!entry) return info;
    Path path = entry->real_path();
    i32 w, h, comp;
    void *_data = stbi_load(path.data(), &w, &h, &comp, force_rgba ? 4 : 0);
    info.data = _data;
    info.image.w = w;
    info.image.h = h;
    info.image.pixel_size = comp;
    return info;
}

Ref<Resource> ResourceLoader::load_texture(ResourceLoader &loader,
                                           ResourceConfiguration &config,
                                           Ref<File> data) {
    Ref<Texture> texture;
    int w, h, comp;

    void *_data = stbi_load(data->get_fullpath().data(), &w, &h, &comp, 0);
    PixelFormat format = comp == 1   ? PixelFormat::R
                         : comp == 2 ? PixelFormat::RG
                         : comp == 3 ? PixelFormat::RGB
                                     : PixelFormat::RGBA;

    if (!_data) {
        spdlog::warn("Can't load texture from {}", data->get_fullpath());
        return ref_cast<Resource>(texture);
    }
    texture.create(TextureType::TEXTURE_2D, w, h, PixelFormat::RGBA,
                   SamplerProperty{}, (const u8 *)_data);

    stbi_image_free(_data);
    return ref_cast<Resource>(texture);
}

Ref<TextureArray> load_texture_array(ResourceLoader &loader,
                                     ResourceConfiguration &config,
                                     const std::string &name) {
    Ref<TextureArray> texture;
    auto &j = config.get_json()[name];
    std::vector<RHI::UpdateBufferInfo> infos;
    for (auto &tex : j) {
        UUID uuid = tex;
        RHI::UpdateBufferInfo info = loader.load_image_to_upload(uuid);
        if (info.data == nullptr) {
            SPDLOG_WARN("Can't load image '{}'", uuid.to_string());
            continue;
        }
        infos.push_back(info);
    }
    if (infos.size() == 0) {
        return texture;
    }
    u32 w = infos[0].image.w;
    u32 h = infos[0].image.h;
    texture.create(TextureType::TEXTURE_2D, w, h, infos.size(),
                   PixelFormat::RGBA, SamplerProperty{});
    u32 i = 0;
    for (RHI::UpdateBufferInfo &info : infos) {
        RHI::update_from_heap(texture->get_handle(), i, 0, 0, info);
        i++;
    }
    return texture;
}

Ref<TextureCubemap> ResourceLoader::load_cubemap(u32 w, u32 h, UUID right,
                                                 UUID left, UUID top,
                                                 UUID bottom, UUID front,
                                                 UUID back) {
    Ref<TextureCubemap> texture;
    RHI::UpdateBufferInfo infos[6];
    infos[0] = load_image_to_upload(right, true);
    infos[1] = load_image_to_upload(left, true);
    infos[2] = load_image_to_upload(top, true);
    infos[3] = load_image_to_upload(bottom, true);
    infos[4] = load_image_to_upload(front, true);
    infos[5] = load_image_to_upload(back, true);

    texture.create(w, h, PixelFormat::RGBA, SamplerProperty{});
    RHI::update_from_heap(texture->get_handle(), (u32)CubemapFace::RIGHT, 0, 0,
                          infos[0]);
    RHI::update_from_heap(texture->get_handle(), (u32)CubemapFace::LEFT, 0, 0,
                          infos[1]);
    RHI::update_from_heap(texture->get_handle(), (u32)CubemapFace::TOP, 0, 0,
                          infos[2]);
    RHI::update_from_heap(texture->get_handle(), (u32)CubemapFace::BOTTOM, 0, 0,
                          infos[3]);
    RHI::update_from_heap(texture->get_handle(), (u32)CubemapFace::FRONT, 0, 0,
                          infos[4]);
    RHI::update_from_heap(texture->get_handle(), (u32)CubemapFace::BACK, 0, 0,
                          infos[5]);
    return texture;
}

Ref<Resource> ResourceLoader::load_mappable_texture(
    ResourceLoader &loader, ResourceConfiguration &config, Ref<File> data) {
    Ref<MappableTexture> texture;
    int w, h, comp;
    void *_data = stbi_load(data->get_fullpath().data(), &w, &h, &comp, 0);
    PixelFormat format = comp == 1   ? PixelFormat::R
                         : comp == 2 ? PixelFormat::RG
                         : comp == 3 ? PixelFormat::RGB
                                     : PixelFormat::RGBA;

    if (!_data) {
        spdlog::warn("Can't load texture from {}", data->get_fullpath());
        return ref_cast<Resource>(texture);
    }
    texture.create(TextureType::TEXTURE_2D, w, h, format, (const u8 *)_data);

    stbi_image_free(_data);
    return ref_cast<Resource>(texture);
}

Ref<Resource> ResourceLoader::load_image(ResourceLoader &loader,
                                         ResourceConfiguration &config,
                                         Ref<File> data) {
    Ref<Image> image = Image::load_from_file(data->get_fullpath());

    return ref_cast<Resource>(image);
}

Ref<Resource> ResourceLoader::load_terrain(ResourceLoader &loader,
                                           ResourceConfiguration &config,
                                           Ref<File> data) {
    Ref<Terrain> terrain;
    auto &terrain_info = config.get_json();
    u32 width = terrain_info["width"];
    u32 height = terrain_info["height"];
    Ref<Image> height_map;
    Ref<Texture> splat_map, light_map;
    auto jheight_map = terrain_info["height_map"];
    height_map = loader.load<Image>(jheight_map);

    auto jsplat_map = terrain_info["splat_map"];
    splat_map = loader.load<Texture>(jsplat_map);
    if (terrain_info.contains("light_map")) {
        auto jlight_map = terrain_info["light_map"];
        light_map = loader.load<Texture>(jlight_map);
    }
    terrain.create(height_map, light_map, splat_map);
    if (terrain_info.contains("tex1")) {
        auto jtex1 = terrain_info["tex1"];
        auto texture = loader.load<Texture>(jtex1);
        terrain->get_material()->set_texture("tex1", texture);
        texture->update_sampler(SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                                .wrap_v = SamplerWrap::REPEAT});
    }

    if (terrain_info.contains("tex1_normal")) {
        auto jtex1 = terrain_info["tex1_normal"];
        auto texture = loader.load<Texture>(jtex1);
        terrain->get_material()->set_texture("tex1_normal", texture);
        texture->update_sampler(SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                                .wrap_v = SamplerWrap::REPEAT});
    }
    return ref_cast<Resource>(terrain);
}

Ref<Resource> ResourceLoader::load_world(ResourceLoader &loader,
                                         ResourceConfiguration &config,
                                         Ref<File> data) {
    Ref<Sky> sky;
    Ref<Terrain> terrain;
    auto &world_info = config.get_json();
    // auto sky_cubemap = loader.load_cubemap(loader, config, "sky");
    // sky.create(sky_cubemap);
    u32 width = world_info["width"];
    u32 height = world_info["height"];
    Ref<Image> height_map;
    Ref<Texture> splat_map, light_map;
    auto jheight_map = world_info["height_map"];
    height_map = loader.load<Image>(jheight_map);

    auto jsplat_map = world_info["splat_map"];
    splat_map = loader.load<Texture>(jsplat_map);
    if (world_info.contains("light_map")) {
        auto jlight_map = world_info["light_map"];
        light_map = loader.load<Texture>(jlight_map);
    }
    terrain.create(height_map, light_map, splat_map);
    if (world_info.contains("tex1")) {
        auto jtex1 = world_info["tex1"];
        auto texture = loader.load<Texture>(jtex1);
        terrain->get_material()->set_texture("tex1", texture);
        texture->update_sampler(SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                                .wrap_v = SamplerWrap::REPEAT});
    }

    if (world_info.contains("tex1_normal")) {
        auto jtex1 = world_info["tex1_normal"];
        auto texture = loader.load<Texture>(jtex1);
        terrain->get_material()->set_texture("tex1_normal", texture);
        texture->update_sampler(SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                                .wrap_v = SamplerWrap::REPEAT});
    }
    return ref_cast<Resource>(terrain);
}

ResourceLoader::ResourceLoader() {
    instance = this;
    spdlog::info("Initializing Resource loader");
    register_type<Shader>(load_shader, true);
    register_type<BasicModel>(load_basic_model, true);
    register_type<SkeletonModel>(load_skeleton_model, true);
    register_type<Texture>(load_texture, true);
    register_type<MappableTexture>(load_mappable_texture, true);
    register_type<Image>(load_image, true);
    register_type<Terrain>(load_terrain);
    register_type<World>(load_world);
}

void ResourceLoader::register_resource(Resource *res) {
    if (res == nullptr || res->get_uuid().is_null()) return;
    this->res_cache[res->get_uuid()] = res;
}

void ResourceLoader::unregister_resource(Resource *res) {
    if (res == nullptr || res->get_uuid().is_null()) return;
    this->res_cache.erase(res->get_uuid());
}

}  // namespace Seed