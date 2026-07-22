#include "editor_world.h"

#include <algorithm>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "core/engine.h"
#include "core/resource/image.h"
#include "core/resource/resource_loader.h"
#include "core/serialize/json_impl.h"
#include "editor/editor.h"

namespace Seed {

EditorWorld::EditorWorld(ResourceEntry *entry) : entry(entry) {
    if (entry != nullptr) {
        config = &entry->config;
    }
    setting.create();
    terrain.create();
    reload();
}

void EditorWorld::copy_sky_setting_to_editor(const SkySetting &setting) {
    sky.up = setting.up;
    sky.down = setting.down;
    sky.left = setting.left;
    sky.right = setting.right;
    sky.front = setting.front;
    sky.back = setting.back;

    ResourceLoader *loader = System::gResourceLoader;
    if (loader == nullptr) return;
    sky.cubemap = loader->load_cubemap(2048, 2048, sky.right, sky.left, sky.up,
                                       sky.down, sky.front, sky.back);
    if (sky.cubemap.is_null()) return;
    sky.sky.create(sky.cubemap);
}

void EditorWorld::copy_editor_sky_to_setting(SkySetting &setting) {
    setting.up = sky.up;
    setting.down = sky.down;
    setting.left = sky.left;
    setting.right = sky.right;
    setting.front = sky.front;
    setting.back = sky.back;
}

void EditorWorld::update_skybox_face(UUID uuid, CubemapFace face) {
    if (uuid.is_null() || sky.cubemap.is_null()) return;

    Ref<Image> image = System::gResourceLoader->load_image(uuid, true);
    if (image.is_null()) return;
    sky.cubemap->update_face(sky.cubemap->get_width(),
                             sky.cubemap->get_height(), face, image);
}

bool EditorWorld::terrain_chunk_exists_at(i32 x, i32 y) const {
    return !terrain.is_null() && terrain->chunk_exists_at(x, y);
}

bool EditorWorld::has_dirty_terrain_maps() const {
    return !terrain.is_null() && terrain->has_dirty_maps();
}

void EditorWorld::save_dirty_terrain_maps() {
    if (setting.is_null() || terrain.is_null()) return;
    terrain->save_dirty_maps(setting->chunks);
}

bool EditorWorld::add_new_chunk(i32 x, i32 y) {
    if (setting.is_null() || terrain.is_null() || System::gEditor == nullptr) {
        return false;
    }
    if (terrain_chunk_exists_at(x, y)) return false;

    ResourceEntry *heightmap_entry = System::gEditor->create_internal_asset(
        fmt::format("{}_{}_{}.png", setting->name, x, y), type_id<Texture>());
    if (heightmap_entry == nullptr) return false;

    ResourceEntry *controlmap_entry = System::gEditor->create_internal_asset(
        fmt::format("{}_{}_{}_control.png", setting->name, x, y),
        type_id<Texture>());
    if (controlmap_entry == nullptr) {
        System::gEditor->remove_asset(heightmap_entry->uuid);
        return false;
    }

    Ref<Image> heightmap;
    heightmap.create(PixelFormat::RG, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);
    heightmap->fill(Color{64, 64}, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);

    Ref<Image> controlmap;
    controlmap.create(PixelFormat::RGBA, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);
    controlmap->fill(Color{0, 0, 0, 0}, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);

    u32 chunk_index = (u32)setting->chunks.size();
    ChunkSetting chunk;
    chunk.x = x;
    chunk.y = y;
    chunk.height_map = heightmap_entry->uuid;
    chunk.control_map = controlmap_entry->uuid;
    setting->chunks.push_back(chunk);

    terrain->add_chunk(chunk.x, chunk.y, heightmap, controlmap);
    terrain->connect_chunk(chunk_index);
    heightmap->save_disk(heightmap_entry->real_path());
    controlmap->save_disk(controlmap_entry->real_path());
    return true;
}

void EditorWorld::read_chunk_controlmaps_from_config() {
    if (config == nullptr || setting.is_null()) return;

    nlohmann::ordered_json &j = config->get_json();
    if (!j.contains("chunks") || !j["chunks"].is_array()) return;

    u32 count = std::min((u32)setting->chunks.size(), (u32)j["chunks"].size());
    for (u32 i = 0; i < count; i++) {
        if (!j["chunks"][i].is_object()) continue;
        setting->chunks[i].control_map =
            j["chunks"][i].value("control_map", UUID{});
    }
}

void EditorWorld::read_terrain_textures_from_config() {
    if (config == nullptr || setting.is_null()) return;

    nlohmann::ordered_json &j = config->get_json();
    setting->terrain_textures =
        j.value("terrain_textures", std::vector<UUID>{});
    setting->terrain_normals = j.value("terrain_normals", std::vector<UUID>{});
    normalize_terrain_palette_size();
}

void EditorWorld::normalize_terrain_palette_size() {
    if (setting.is_null()) return;
    setting->terrain_normals.resize(setting->terrain_textures.size());
}

void EditorWorld::upload_terrain_palette_to_gpu() {
    if (setting.is_null() || terrain.is_null()) return;

    ResourceLoader *loader = System::gResourceLoader;
    if (loader == nullptr) return;

    if (setting->terrain_textures.empty()) {
        terrain->reset_texture_palette();
        return;
    }

    const u32 texture_count = std::min((u32)setting->terrain_textures.size(),
                                       (u32)TERRAIN_TEXTURE_LAYERS);
    for (u32 i = 0; i < texture_count; i++) {
        const UUID texture_uuid = setting->terrain_textures[i];
        RHI::UpdateBufferInfo texture_info{};
        if (!texture_uuid.is_null()) {
            texture_info = loader->load_image_to_upload(texture_uuid, true);
        }
        terrain->update_texture_layer(i, texture_info);

        const UUID normal_uuid = setting->terrain_normals[i];
        RHI::UpdateBufferInfo normal_info{};
        if (!normal_uuid.is_null()) {
            normal_info = loader->load_image_to_upload(normal_uuid, true);
        }
        terrain->update_normal_layer(i, normal_info);
    }
}

void EditorWorld::clear_tiles() {
    if (setting.is_null()) return;
    setting->chunks.clear();
    terrain->clear_chunks();
}

void EditorWorld::reload() {
    sky = {};
    static_models.clear();
    terrain->clear_chunks();
    if (entry == nullptr) return;

    ResourceLoader *loader = System::gResourceLoader;
    if (loader == nullptr) return;

    Ref<WorldSetting> loaded = loader->load<WorldSetting>(entry->uuid);
    if (!loaded.is_null()) {
        setting = loaded;
    }

    read_chunk_controlmaps_from_config();
    read_terrain_textures_from_config();
    upload_terrain_palette_to_gpu();

    copy_sky_setting_to_editor(setting->sky);
    apply_directional_light_to_runtime();

    for (u32 i = 0; i < setting->chunks.size(); i++) {
        ChunkSetting &chunk = setting->chunks[i];
        Ref<Image> heightmap = loader->load_image(chunk.height_map);
        Ref<Image> controlmap = loader->load_image(chunk.control_map);
        terrain->add_chunk(chunk.x, chunk.y, heightmap, controlmap);
    }

    terrain->sync_loaded_tile_seams();
    terrain->clear_dirty_maps();
    rebuild_static_model_instances();
}

void EditorWorld::save() {
    if (config == nullptr || setting.is_null()) return;
    copy_editor_sky_to_setting(setting->sky);
    normalize_terrain_palette_size();

    nlohmann::ordered_json &j = config->get_json();
    if (!j.is_object()) j = nlohmann::ordered_json::object();
    j["name"] = setting->name;

    nlohmann::ordered_json sky_json = nlohmann::ordered_json::object();
    sky_json["up"] = setting->sky.up;
    sky_json["down"] = setting->sky.down;
    sky_json["left"] = setting->sky.left;
    sky_json["right"] = setting->sky.right;
    sky_json["front"] = setting->sky.front;
    sky_json["back"] = setting->sky.back;
    j["sky"] = sky_json;

    nlohmann::ordered_json light_json = nlohmann::ordered_json::object();
    light_json["direction"] = setting->dir_light.direction;
    light_json["diffuse"] = setting->dir_light.diffuse;
    light_json["specular"] = setting->dir_light.specular;
    j["directional_light"] = light_json;

    j["terrain_textures"] = setting->terrain_textures;
    j["terrain_normals"] = setting->terrain_normals;
    j["chunks"] = nlohmann::ordered_json::array();

    for (const ChunkSetting &chunk : setting->chunks) {
        nlohmann::ordered_json chunk_json = nlohmann::ordered_json::object();
        chunk_json["x"] = chunk.x;
        chunk_json["y"] = chunk.y;
        chunk_json["height_map"] = chunk.height_map;
        chunk_json["control_map"] = chunk.control_map;
        chunk_json["position_lights"] = nlohmann::ordered_json::array();
        chunk_json["static_objects"] = nlohmann::ordered_json::array();

        for (const PointLightSetting &light : chunk.lights) {
            nlohmann::ordered_json point_light_json =
                nlohmann::ordered_json::object();
            point_light_json["position"] = light.position;
            point_light_json["diffuse"] = light.diffuse;
            point_light_json["specular"] = light.specular;
            chunk_json["position_lights"].push_back(point_light_json);
        }

        for (const StaticObjectSetting &object : chunk.static_objects) {
            nlohmann::ordered_json object_json =
                nlohmann::ordered_json::object();
            object_json["name"] = object.name;
            object_json["x"] = object.x;
            object_json["y"] = object.y;
            object_json["z"] = object.z;
            object_json["model"] = object.model;
            chunk_json["static_objects"].push_back(object_json);
        }

        j["chunks"].push_back(chunk_json);
    }
}

bool EditorWorld::update_static_model_instance(u32 chunk_index,
                                               u32 object_index) {
    if (setting.is_null() || chunk_index >= setting->chunks.size()) {
        return false;
    }

    ChunkSetting &chunk = setting->chunks[chunk_index];
    if (object_index >= chunk.static_objects.size()) return false;

    StaticObjectSetting &object = chunk.static_objects[object_index];
    if (object.model.is_null()) return false;

    EditorStaticModel &entry =
        static_models[std::pair<u32, u32>(chunk_index, object_index)];
    if (entry.model.is_null()) {
        entry.model = System::gResourceLoader->load<BasicModel>(object.model);
        if (entry.model.is_null()) return false;
    }
    if (entry.instance.is_null()) {
        entry.instance = entry.model->create_instance();
    }

    entry.instance->clear();
    Transform transform;
    transform.set_position((f32)object.x, (f32)object.y, (f32)object.z);
    entry.model->add_instance(entry.instance, transform);
    return true;
}

void EditorWorld::rebuild_static_model_instances() {
    static_models.clear();
    if (setting.is_null()) return;

    for (u32 chunk_index = 0; chunk_index < setting->chunks.size();
         chunk_index++) {
        ChunkSetting &chunk = setting->chunks[chunk_index];
        for (u32 object_index = 0; object_index < chunk.static_objects.size();
             object_index++) {
            update_static_model_instance(chunk_index, object_index);
        }
    }
}

void EditorWorld::apply_directional_light_to_runtime() {
    if (setting.is_null()) return;
    SeedEngine *engine = System::gEngine;
    if (engine == nullptr || engine->get_world() == nullptr) return;

    engine->get_world()->get_direction_light() = DirectionalLight(
        setting->dir_light.direction, setting->dir_light.diffuse,
        setting->dir_light.specular, true);
}

}  // namespace Seed
