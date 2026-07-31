#include "resource_loader.h"
#include "core/container/kstring.h"
#include "core/io/file.h"
#include "core/io/dir.h"
#include <spdlog/spdlog.h>
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/macro.h"
#include "core/ref.h"
#include "core/rendering/mesh.h"

#include <nlohmann/json.hpp>
#include <vector>
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/default_storage.h"
#include "core/resource/resource_entry.h"
#include "core/serialize/json_impl.h"

#include "core/resource/model.h"
#include "core/resource/texture.h"
#include "core/resource/image.h"
#include "core/types.h"
#include "resource.h"
#include "core/resource/shader.h"
#include "core/resource/world_setting.h"
#include "core/gui/gui_engine.h"
#include "core/gui/gui.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Seed {

ResourceLoader::~ResourceLoader() {}

void ResourceLoader::handle_async_notifies() {
    while (!notifies.is_empty()) {
        auto notify = notifies.peek();
        notify();
        notifies.pop();
    }
}

Ref<Resource> ResourceLoader::load_shader(ResourceLoader &loader,
                                          ResourceConfiguration &config,
                                          Ref<File> data) {
    Ref<Shader> shader;
    KString shader_code;
    shader_code = data->read_str();
    shader.create(data->get_fullpath(), shader_code);
    return ref_cast<Resource>(shader);
}

void ResourceLoader::load_meshes(ResourceLoader &loader,
                                 ResourceConfiguration &config, Ref<File> data,
                                 std::vector<Ref<Mesh>> &meshes,
                                 Ref<Skeleton> skeleton,
                                 std::vector<Ref<Animation>> &animations) {
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
            meshes.push_back(
                Ref<Mesh>(&System::gDefaultStorage->skeleton_mesh_desc,
                          vertices, indices, (AABB)jmesh["bounding_box"]));
        } else {
            std::vector<ModelVertex> vertices;
            data->read_vector(vertices, jmesh["vertex_count"]);
            data->read_vector(indices, jmesh["index_count"]);
            meshes.push_back(Ref<Mesh>(&System::gDefaultStorage->mesh_desc,
                                       vertices, indices,
                                       (AABB)jmesh["bounding_box"]));
        }

        mesh_mats.push_back(jmesh["material_id"]);
    }

    if (skeleton.is_valid() && model_info.contains("bones")) {
        auto jbones = model_info["bones"];
        u64 bin_offset = jbones["bin_offset"];
        u32 bone_cnt = (u64)jbones["bin_size"] / sizeof(Mat4);
        skeleton->bones.resize(bone_cnt);
        data->read_vector(skeleton->bones, bone_cnt);
        skeleton->bone_parents = jbones["parents"].get<std::vector<u16>>();
    }

    auto jmaterials = model_info["materials"];
    for (auto &jmaterial : jmaterials) {
        Ref<BaseMaterial> mat;
        if (skeleton.is_valid()) {
            mat.create(System::gDefaultStorage->skeleton_mesh_shader);
        } else {
            mat.create();
        }
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
            data->read(&clip_info);
            animation->clips.push_back({});
            AnimationClip &clip = animation->clips.back();
            clip.bone_id = clip_info.bone_id;
            data->read_vector(clip.position_keys, clip_info.position_key_count);
            data->read_vector(clip.rotation_keys, clip_info.rotation_key_count);
            data->read_vector(clip.scaling_keys, clip_info.scaling_key_count);
        }
        animations.push_back(animation);
    }
}

Ref<Resource> ResourceLoader::load_basic_model(ResourceLoader &loader,
                                               ResourceConfiguration &config,
                                               Ref<File> data) {
    Ref<BasicModel> model;
    std::vector<Ref<Mesh>> meshes;
    std::vector<Ref<Animation>> animations;
    load_meshes(loader, config, data, meshes, Ref<Skeleton>(), animations);
    model.create(meshes);
    return ref_cast<Resource>(model);
}

Ref<Resource> ResourceLoader::load_skeleton_model(ResourceLoader &loader,
                                                  ResourceConfiguration &config,
                                                  Ref<File> data) {
    Ref<SkeletonModel> model;
    std::vector<Ref<Mesh>> meshes;
    std::vector<Ref<Animation>> animations;

    auto model_info = config.get_json();

    Ref<Skeleton> skeleton;
    skeleton.create();
    load_meshes(loader, config, data, meshes, skeleton, animations);

    model.create(meshes, skeleton);

    for (Ref<Animation> animation : animations) {
        model->add_animation(animation);
    }
    return ref_cast<Resource>(model);
}
/* since we now use malloc in update heap, we do not need to free here */
RHI::UpdateBufferInfo ResourceLoader::load_image_to_upload(UUID uuid,
                                                           bool force_rgba) {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    RHI::UpdateBufferInfo info;
    info.data = nullptr;
    if (!entry) return info;
    Path path = entry->real_path();
    i32 w, h, comp;
    void *_data = stbi_load(path.data(), &w, &h, &comp, force_rgba ? 4 : 0);
    comp = force_rgba ? 4 : comp;
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

    void *_data = stbi_load(data->get_fullpath().data(), &w, &h, &comp, 4);

    if (!_data) {
        SEED_WARN("Can't load texture from {}, skipping.",
                  data->get_fullpath());
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
            SEED_WARN("Can't load image '{}'", uuid.to_string());
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
    /* fill empty cubemap */
    for (u32 i = 0; i < 6; i++) {
        if (!infos[i].data) {
            infos[i].data = malloc(w * h * 4);
            infos[i].image.w = w;
            infos[i].image.h = h;
            infos[i].image.pixel_size = 4;
            memset(infos[i].data, 255, w * h * 4);
        }
    }

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
        SEED_WARN("Can't load texture from {}", data->get_fullpath());
        return ref_cast<Resource>(texture);
    }
    texture.create(TextureType::TEXTURE_2D, w, h, format, (const u8 *)_data);

    stbi_image_free(_data);
    return ref_cast<Resource>(texture);
}

Ref<Image> ResourceLoader::load_image(UUID uuid, bool force_rgba) {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    Ref<Image> image;
    if (!entry) return image;
    Path path = entry->real_path();
    image = Image::load_from_file(path, force_rgba);

    return image;
}

Ref<Resource> ResourceLoader::load_world(ResourceLoader &loader,
                                         ResourceConfiguration &config,
                                         Ref<File> data) {
    auto &j = config.get_json();
    Ref<WorldSetting> world;
    world.create();
    world->name = j.value<KString>("name", "");
    world->terrain_textures = j.value("terrain_textures", std::vector<UUID>{});
    world->terrain_normals = j.value("terrain_normals", std::vector<UUID>{});

    auto read_sky = [&](const nlohmann::json &j) -> SkySetting {
        SkySetting sky;
        sky.up = j.value("up", UUID{});
        sky.down = j.value("down", UUID{});
        sky.left = j.value("left", UUID{});
        sky.right = j.value("right", UUID{});
        sky.front = j.value("front", UUID{});
        sky.back = j.value("back", UUID{});
        return sky;
    };

    if (j.contains("sky")) {
        world->sky = read_sky(j["sky"]);
    };

    if (j.contains("directional_light")) {
        auto &dir_j = j["directional_light"];
        world->dir_light.direction =
            dir_j.value("direction", Vec3{-0.5, -0.5, 0});
        world->dir_light.diffuse = dir_j.value("diffuse", Vec3{0.8, -0.8, 0.8});
        world->dir_light.specular =
            dir_j.value("specular", Vec3{0.4f, 0.4f, 0.4f});
    }

    auto read_point_light = [&](const nlohmann::json &j) -> PointLightSetting {
        PointLightSetting light;
        if (!j.is_object()) return light;
        light.position = j.value("position", light.position);
        light.diffuse = j.value("diffuse", light.diffuse);
        light.specular = j.value("specular", light.specular);
        return light;
    };

    auto read_static_object =
        [&](const nlohmann::json &j) -> StaticObjectSetting {
        StaticObjectSetting object;
        if (!j.is_object()) return object;
        object.name = KString(j.value<std::string>("name", ""));
        object.x = j.value("x", 0);
        object.y = j.value("y", 0);
        object.z = j.value("z", object.y);
        if (!j.contains("z")) object.y = 0;
        object.model = j.value("model", UUID{});
        return object;
    };

    auto read_chunk = [&](const nlohmann::json &j) -> ChunkSetting {
        ChunkSetting chunk;
        if (!j.is_object()) return chunk;
        chunk.x = j.value("x", 0);
        chunk.y = j.value("y", 0);
        chunk.height_map = j.value("height_map", UUID{});
        chunk.control_map = j.value("control_map", UUID{});

        if (j.contains("position_lights") && j["position_lights"].is_array()) {
            for (const auto &light_j : j["position_lights"]) {
                chunk.lights.push_back(read_point_light(light_j));
            }
        }
        if (j.contains("static_objects") && j["static_objects"].is_array()) {
            for (const auto &object_j : j["static_objects"]) {
                chunk.static_objects.push_back(read_static_object(object_j));
            }
        }
        return chunk;
    };
    if (j.contains("chunks") && j["chunks"].is_array()) {
        for (const auto &chunk_j : j["chunks"]) {
            world->chunks.push_back(read_chunk(chunk_j));
        }
    }
    return ref_cast<Resource>(world);
}

Ref<Resource> ResourceLoader::load_ui(ResourceLoader &loader,
                                      ResourceConfiguration &config,
                                      Ref<File> data) {
    Ref<GuiDocument> document;
    document.create();
    document->source = data->get_path();
    document->content = data->read_str();
    if (document->content.is_empty()) return Ref<Resource>();
    return ref_cast<Resource>(document);
}
ResourceLoader::ResourceLoader() {
    spdlog::info("Initializing Resource loader");
    register_type<Shader>(load_shader, true);
    register_type<BasicModel>(load_basic_model, true);
    register_type<SkeletonModel>(load_skeleton_model, true);
    register_type<Texture>(load_texture, true);
    register_type<MappableTexture>(load_mappable_texture, true);
    register_type<WorldSetting>(load_world);
    register_type<GuiDocument>(load_ui, true);
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
