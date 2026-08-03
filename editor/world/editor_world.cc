#include "editor_world.h"

#include <fmt/format.h>

#include "core/engine.h"
#include "core/resource/image.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/serialize/json_impl.h"
#include "core/world/world.h"
#include "editor/editor.h"

namespace Seed {

EditorWorld::EditorWorld(ResourceEntry *entry) : entry(entry) {
    world = System::gEngine->get_world();
    terrain.create();
    reload();
}

World *EditorWorld::get_world() const { return world; }

void EditorWorld::register_editor_components() {
    world->ecs().on_add<MeshInstance>(
        [&](EntityManager &m, Entity e, MeshInstance *ph) {
            if (ph->model.is_null()) return;
            Ref<Model> _model = ref_cast<Model>(ph->model);
            if (!world->is_model_instance_added(_model)) {
                Ref<InstanceBatch> instance = ph->model->create_instance();
                world->register_model_instance(_model, instance);
            }
        });
}

Ref<WorldSetting> EditorWorld::get_setting() const {
    return world->get_setting();
}

std::vector<ChunkSetting> &EditorWorld::get_chunks() {
    return world->get_setting()->chunks;
}

bool EditorWorld::update_skybox_face(UUID uuid, CubemapFace face) {
    if (uuid.is_null() || System::gResourceLoader == nullptr) return false;

    Ref<Sky> sky = get_world()->get_sky();
    if (sky.is_null()) return false;

    Ref<TextureCubemap> cubemap = sky->get_cubemap();
    if (cubemap.is_null()) return false;

    Ref<Image> image = System::gResourceLoader->load_image(uuid, true);
    if (image.is_null()) return false;

    cubemap->update_face(cubemap->get_width(), cubemap->get_height(), face,
                         image);

    SkySetting &setting = get_setting()->sky;
    switch (face) {
        case CubemapFace::TOP:
            setting.up = uuid;
            break;
        case CubemapFace::BOTTOM:
            setting.down = uuid;
            break;
        case CubemapFace::LEFT:
            setting.left = uuid;
            break;
        case CubemapFace::RIGHT:
            setting.right = uuid;
            break;
        case CubemapFace::FRONT:
            setting.front = uuid;
            break;
        case CubemapFace::BACK:
            setting.back = uuid;
            break;
    }
    return true;
}

bool EditorWorld::update_terrain_texture(u32 layer, UUID uuid,
                                         TerrainTextureKind kind) {
    if (terrain.is_null() || uuid.is_null() ||
        layer >= TERRAIN_TEXTURE_LAYERS || System::gResourceLoader == nullptr) {
        return false;
    }

    RHI::UpdateBufferInfo info =
        System::gResourceLoader->load_image_to_upload(uuid, true);
    if (info.data == nullptr) return false;

    if (kind == TerrainTextureKind::Diffuse) {
        if (!terrain->update_texture_layer(layer, info)) return false;
    } else if (!terrain->update_normal_layer(layer, info)) {
        return false;
    }

    Ref<WorldSetting> setting = get_setting();
    const u32 palette_size = layer + 1;
    if (setting->terrain_textures.size() < palette_size) {
        setting->terrain_textures.resize(palette_size);
    }
    if (setting->terrain_normals.size() < palette_size) {
        setting->terrain_normals.resize(palette_size);
    }

    if (kind == TerrainTextureKind::Diffuse) {
        setting->terrain_textures[layer] = uuid;
        if (setting->terrain_normals[layer].is_null()) {
            terrain->update_normal_layer(layer, {});
        }
    } else {
        setting->terrain_normals[layer] = uuid;
        if (setting->terrain_textures[layer].is_null()) {
            terrain->update_texture_layer(layer, {});
        }
    }
    return true;
}

bool EditorWorld::terrain_chunk_exists_at(i32 x, i32 y) const {
    return !terrain.is_null() && terrain->chunk_exists_at(x, y);
}

void EditorWorld::save_dirty_terrain_maps() {
    if (terrain.is_null()) return;
    terrain->save_dirty_maps(get_setting()->chunks);
}

bool EditorWorld::add_new_chunk(i32 x, i32 y) {
    if (terrain.is_null() || System::gEditor == nullptr ||
        get_setting()->chunks.size() >= TERRAIN_CHUNK_LAYERS ||
        terrain_chunk_exists_at(x, y)) {
        return false;
    }

    Ref<WorldSetting> setting = get_setting();
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

    Ref<Image> heightmap = Terrain::create_default_heightmap();
    Ref<Image> controlmap = Terrain::create_default_controlmap();

    ChunkSetting chunk;
    chunk.x = x;
    chunk.y = y;
    chunk.height_map = heightmap_entry->uuid;
    chunk.control_map = controlmap_entry->uuid;
    setting->chunks.push_back(chunk);

    const u32 chunk_index = (u32)setting->chunks.size() - 1;
    terrain->add_chunk(chunk.x, chunk.y, heightmap, controlmap);
    terrain->connect_chunk(chunk_index);
    heightmap->save_disk(heightmap_entry->real_path());
    controlmap->save_disk(controlmap_entry->real_path());
    return true;
}

void EditorWorld::clear_tiles() {
    if (terrain.is_null()) return;
    get_setting()->chunks.clear();
    terrain->clear_chunks();
    rebuild_static_models();
}

void EditorWorld::reload() {
    Ref<WorldSetting> loaded_setting;
    if (entry != nullptr && System::gResourceLoader != nullptr) {
        loaded_setting =
            System::gResourceLoader->load<WorldSetting>(entry->uuid);
    }
    if (loaded_setting.is_null()) loaded_setting.create();

    world->load_setting(loaded_setting);
    terrain->load_chunks(loaded_setting->chunks);
    rebuild_static_models();
}

void EditorWorld::rebuild_static_models() { objects.clear(); }

void EditorWorld::apply_directional_light() {
    const DirectionalLightSetting &setting = get_setting()->dir_light;
    world->get_direction_light() = DirectionalLight(
        setting.direction, setting.diffuse, setting.specular, true);
}

void EditorWorld::save() const {
    if (entry == nullptr) return;

    Ref<WorldSetting> setting = get_setting();
    nlohmann::ordered_json &json = entry->config.get_json();
    if (!json.is_object()) json = nlohmann::ordered_json::object();

    json["name"] = setting->name;
    json["sky"] = {{"up", setting->sky.up},       {"down", setting->sky.down},
                   {"left", setting->sky.left},   {"right", setting->sky.right},
                   {"front", setting->sky.front}, {"back", setting->sky.back}};
    json["directional_light"] = {{"direction", setting->dir_light.direction},
                                 {"diffuse", setting->dir_light.diffuse},
                                 {"specular", setting->dir_light.specular}};
    json["terrain_textures"] = setting->terrain_textures;
    json["terrain_normals"] = setting->terrain_normals;
    json["chunks"] = nlohmann::ordered_json::array();

    for (const ChunkSetting &chunk : setting->chunks) {
        nlohmann::ordered_json chunk_json = {
            {"x", chunk.x},
            {"y", chunk.y},
            {"height_map", chunk.height_map},
            {"control_map", chunk.control_map},
            {"position_lights", nlohmann::ordered_json::array()},
            {"static_objects", nlohmann::ordered_json::array()}};

        for (const PointLightSetting &light : chunk.lights) {
            chunk_json["position_lights"].push_back(
                {{"position", light.position},
                 {"diffuse", light.diffuse},
                 {"specular", light.specular}});
        }

        for (const StaticObjectSetting &object : chunk.static_objects) {
            chunk_json["static_objects"].push_back({{"name", object.name},
                                                    {"x", object.x},
                                                    {"y", object.y},
                                                    {"z", object.z},
                                                    {"model", object.model}});
        }

        json["chunks"].push_back(std::move(chunk_json));
    }
}

}  // namespace Seed
