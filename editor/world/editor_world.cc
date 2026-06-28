#include "editor_world.h"
#include <algorithm>
#include <cstdio>
#include <fmt/format.h>
#include <imgui.h>
#include "core/engine.h"
#include "core/resource/image.h"
#include "core/resource/resource_loader.h"
#include "core/serialize/json_impl.h"
#include "editor/editor.h"
#include "editor/gui/editor_ui.h"

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

    ResourceLoader *loader = ResourceLoader::get_instance();
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

    Ref<Image> image = ResourceLoader::get_instance()->load_image(uuid, true);
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
    if (setting.is_null() || terrain.is_null() || gEditor == nullptr) {
        return false;
    }
    if (terrain_chunk_exists_at(x, y)) return false;

    ResourceEntry *heightmap_entry = gEditor->create_internal_asset(
        fmt::format("{}_{}_{}.png", setting->name, x, y), type_id<Texture>());
    if (heightmap_entry == nullptr) return false;

    ResourceEntry *controlmap_entry = gEditor->create_internal_asset(
        fmt::format("{}_{}_{}_control.png", setting->name, x, y),
        type_id<Texture>());
    if (controlmap_entry == nullptr) {
        gEditor->remove_asset(heightmap_entry->uuid);
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

EditorUI::TexturePreview EditorWorld::build_texture_preview(UUID texture_uuid) {
    EditorUI::TexturePreview preview;
    if (texture_uuid.is_null()) return preview;

    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) {
        preview.failed = true;
        return preview;
    }

    ResourceEntry *texture_entry =
        loader->get_entries().get_entry(texture_uuid);
    if (texture_entry == nullptr ||
        texture_entry->type_id != type_id<Texture>()) {
        preview.failed = true;
        return preview;
    }

    Ref<Image> image = Image::load_from_file(texture_entry->real_path(), true);
    if (image.is_null()) {
        preview.failed = true;
        return preview;
    }

    preview.width = image->get_width();
    preview.height = image->get_height();
    preview.texture = image->create_texture();
    preview.failed = preview.texture.is_null();
    return preview;
}

void EditorWorld::rebuild_terrain_texture_previews() {
    terrain_texture_previews.clear();
    terrain_normal_previews.clear();
    if (setting.is_null()) return;

    normalize_terrain_palette_size();
    terrain_texture_previews.reserve(setting->terrain_textures.size());
    terrain_normal_previews.reserve(setting->terrain_normals.size());
    for (UUID texture_uuid : setting->terrain_textures) {
        terrain_texture_previews.push_back(build_texture_preview(texture_uuid));
    }
    for (UUID normal_uuid : setting->terrain_normals) {
        terrain_normal_previews.push_back(build_texture_preview(normal_uuid));
    }
    upload_terrain_palette_to_gpu();
}

void EditorWorld::upload_terrain_palette_to_gpu() {
    if (setting.is_null() || terrain.is_null()) return;

    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) return;

    Ref<Image> normal_image;
    normal_image.create(PixelFormat::RGBA, TERRAIN_TEXTURE_SIZE,
                        TERRAIN_TEXTURE_SIZE);
    normal_image->fill(Color{128, 128, 255, 255}, TERRAIN_TEXTURE_SIZE,
                       TERRAIN_TEXTURE_SIZE);

    for (u32 i = 0; i < setting->terrain_textures.size(); i++) {
        RHI::UpdateBufferInfo tex_info =
            loader->load_image_to_upload(setting->terrain_textures[i], true);

        terrain->get_material()->get_textures()->update_layer(i, tex_info);

        if (setting->terrain_normals[i].is_null()) {
            terrain->get_material()->get_texture_normals()->update_layer(
                TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE, i,
                normal_image->get_data());
        } else {
            RHI::UpdateBufferInfo norm_info =
                loader->load_image_to_upload(setting->terrain_normals[i], true);
            terrain->get_material()->get_texture_normals()->update_layer(
                i, norm_info);
        }
        terrain_texture_previews[i].upload_failed = false;
        terrain_normal_previews[i].upload_failed = false;
    }
}

void EditorWorld::clear_tiles() {
    if (setting.is_null()) return;
    setting->chunks.clear();
    terrain->clear_chunks();
}

void EditorWorld::reload() {
    sky = {};
    terrain_texture_previews.clear();
    terrain_normal_previews.clear();
    static_models.clear();
    terrain->clear_chunks();
    if (entry == nullptr) return;

    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) return;

    Ref<WorldSetting> loaded = loader->load<WorldSetting>(entry->uuid);
    if (!loaded.is_null()) {
        setting = loaded;
    }

    read_chunk_controlmaps_from_config();
    read_terrain_textures_from_config();
    rebuild_terrain_texture_previews();

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

const EditorUI::TexturePreview *EditorWorld::get_terrain_texture_preview(
    u32 index) const {
    EXPECT_INDEX_INBOUND_RET(index, terrain_texture_previews.size(), nullptr);
    return &terrain_texture_previews[index];
}

const EditorUI::TexturePreview *EditorWorld::get_terrain_normal_preview(
    u32 index) const {
    EXPECT_INDEX_INBOUND_RET(index, terrain_normal_previews.size(), nullptr);
    return &terrain_normal_previews[index];
}

void EditorWorld::add_terrain_texture(UUID texture) {
    if (setting.is_null()) return;
    setting->terrain_textures.push_back(texture);
    setting->terrain_normals.push_back(UUID{});
    rebuild_terrain_texture_previews();
}

bool EditorWorld::set_terrain_texture(u32 index, UUID texture) {
    if (setting.is_null() || index >= setting->terrain_textures.size()) {
        return false;
    }
    setting->terrain_textures[index] = texture;
    rebuild_terrain_texture_previews();
    if (texture.is_null()) return true;
    return index < terrain_texture_previews.size() &&
           !terrain_texture_previews[index].upload_failed;
}

bool EditorWorld::set_terrain_normal(u32 index, UUID texture) {
    if (setting.is_null() || index >= setting->terrain_textures.size()) {
        return false;
    }
    normalize_terrain_palette_size();
    setting->terrain_normals[index] = texture;
    rebuild_terrain_texture_previews();
    if (texture.is_null()) return true;
    return index < terrain_normal_previews.size() &&
           !terrain_normal_previews[index].upload_failed;
}

void EditorWorld::remove_terrain_texture(u32 index) {
    if (setting.is_null() || index >= setting->terrain_textures.size()) return;
    setting->terrain_textures.erase(setting->terrain_textures.begin() + index);
    if (index < setting->terrain_normals.size()) {
        setting->terrain_normals.erase(setting->terrain_normals.begin() +
                                       index);
    }
    rebuild_terrain_texture_previews();
}

EditorTile *EditorWorld::get_tile(u32 index) {
    if (terrain.is_null()) return nullptr;
    return terrain->get_tile(index);
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
        entry.model = ResourceLoader::get_instance()->load<BasicModel>(
            object.model);
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
        for (u32 object_index = 0;
             object_index < chunk.static_objects.size(); object_index++) {
            update_static_model_instance(chunk_index, object_index);
        }
    }
}

void EditorWorld::apply_directional_light_to_runtime() {
    if (setting.is_null()) return;
    SeedEngine *engine = SeedEngine::get_instance();
    if (engine == nullptr || engine->get_world() == nullptr) return;

    engine->get_world()->get_direction_light() = DirectionalLight(
        setting->dir_light.direction, setting->dir_light.diffuse,
        setting->dir_light.specular, true);
}

EditorWorldInspector::EditorWorldInspector(EditorWorld *world) : world(world) {}

void EditorWorldInspector::draw_inspector() {
    if (world == nullptr) return;

    char name_buffer[256] = {};
    const KString &name = world->get_name();
    if (!name.is_empty()) {
        std::snprintf(name_buffer, sizeof(name_buffer), "%s", name.data());
    }
    if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
        world->set_name(name_buffer);
    }

    if (ImGui::CollapsingHeader("Sky", ImGuiTreeNodeFlags_DefaultOpen)) {
        EditorSky &sky = world->get_sky();
        if (sky.cubemap.is_null()) {
            ImGui::TextDisabled("Sky preview unavailable");
        } else {
            if (drag_uuid("up", sky.up)) {
                world->update_skybox_face(sky.up, CubemapFace::TOP);
            }
            if (drag_uuid("down", sky.down)) {
                world->update_skybox_face(sky.down, CubemapFace::BOTTOM);
            }
            if (drag_uuid("left", sky.left)) {
                world->update_skybox_face(sky.left, CubemapFace::LEFT);
            }
            if (drag_uuid("right", sky.right)) {
                world->update_skybox_face(sky.right, CubemapFace::RIGHT);
            }
            if (drag_uuid("front", sky.front)) {
                world->update_skybox_face(sky.front, CubemapFace::FRONT);
            }
            if (drag_uuid("back", sky.back)) {
                world->update_skybox_face(sky.back, CubemapFace::BACK);
            }
        }
    }

    if (ImGui::CollapsingHeader("Directional Light",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        DirectionalLightSetting &light = world->get_directional_light();
        bool changed = EditorUI::draw_vec3("direction", light.direction);
        changed |= EditorUI::draw_vec3("diffuse", light.diffuse);
        changed |= EditorUI::draw_vec3("specular", light.specular);
        if (changed) {
            world->apply_directional_light_to_runtime();
        }
    }

    ImGui::Separator();
    ImGui::Text("Terrain tiles: %zu", world->get_chunks().size());
}

void EditorWorldInspector::save() {
    if (world != nullptr) world->save();
}

EditorStaticObjectInspector::EditorStaticObjectInspector(EditorWorld *world,
                                                         u32 chunk_index,
                                                         u32 object_index)
    : world(world), chunk_index(chunk_index), object_index(object_index) {}

void EditorStaticObjectInspector::draw_inspector() {
    if (world == nullptr) return;
    if (chunk_index >= world->get_chunks().size()) return;

    ChunkSetting &chunk = world->get_chunks()[chunk_index];
    if (object_index >= chunk.static_objects.size()) return;

    StaticObjectSetting &object =
        chunk.static_objects[(u32)object_index];
    char name_buffer[256] = {};
    if (!object.name.is_empty()) {
        std::snprintf(name_buffer, sizeof(name_buffer), "%s",
                      object.name.data());
    }

    if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
        object.name = name_buffer;
    }

    i32 pos[3] = {object.x, object.y, object.z};
    if (ImGui::DragInt3("Position", pos, 1.0f)) {
        object.x = pos[0];
        object.y = pos[1];
        object.z = pos[2];
        world->update_static_model_instance(chunk_index, object_index);
    }

    KString model_text = object.model.to_string();
    ImGui::Text("Model: %s", model_text.data());
}

void EditorStaticObjectInspector::save() {
    if (world != nullptr) world->save();
}

}  // namespace Seed
