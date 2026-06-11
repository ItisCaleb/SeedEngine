#include "editor_world.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <imgui.h>
#include "core/engine.h"
#include "core/serialize/json_impl.h"
#include "core/resource/resource_loader.h"
#include "editor/editor.h"

namespace Seed {

using Json = nlohmann::ordered_json;

static constexpr u32 EDITOR_HEIGHTMAP_INNER_SIZE = 257;
static constexpr u32 EDITOR_HEIGHTMAP_BORDER = 1;
static constexpr u32 EDITOR_HEIGHTMAP_SIZE =
    EDITOR_HEIGHTMAP_INNER_SIZE + EDITOR_HEIGHTMAP_BORDER * 2;
static constexpr u32 EDITOR_HEIGHTMAP_INNER_FIRST = EDITOR_HEIGHTMAP_BORDER;
static constexpr u32 EDITOR_HEIGHTMAP_INNER_LAST =
    EDITOR_HEIGHTMAP_INNER_FIRST + EDITOR_HEIGHTMAP_INNER_SIZE - 1;
static constexpr u32 EDITOR_HEIGHTMAP_GHOST_FIRST = 0;
static constexpr u32 EDITOR_HEIGHTMAP_GHOST_LAST = EDITOR_HEIGHTMAP_SIZE - 1;

static void copy_sky_setting_to_editor(const SkySetting &setting,
                                       EditorSky &sky) {
    sky.up = setting.up;
    sky.down = setting.down;
    sky.left = setting.left;
    sky.right = setting.right;
    sky.front = setting.front;
    sky.back = setting.back;
    sky.cubemap = ResourceLoader::get_instance()->load_cubemap(
        2048, 2048, sky.right, sky.left, sky.up, sky.down, sky.front,
        sky.back);
    sky.sky.create(sky.cubemap);
}

static void copy_editor_sky_to_setting(const EditorSky &sky,
                                       SkySetting &setting) {
    setting.up = sky.up;
    setting.down = sky.down;
    setting.left = sky.left;
    setting.right = sky.right;
    setting.front = sky.front;
    setting.back = sky.back;
}

static i32 find_chunk_index_at(const std::vector<EditorChunk> &chunks, i32 x,
                               i32 y) {
    for (i32 i = 0; i < (i32)chunks.size(); i++) {
        if (chunks[i].x == x && chunks[i].y == y) return i;
    }
    return -1;
}

static bool valid_heightmap(Ref<Image> heightmap) {
    return !heightmap.is_null() &&
           heightmap->get_width() >= EDITOR_HEIGHTMAP_SIZE &&
           heightmap->get_height() >= EDITOR_HEIGHTMAP_SIZE;
}

static void copy_height_pixel(Ref<Image> src, u32 src_x, u32 src_y,
                              Ref<Image> dst, u32 dst_x, u32 dst_y) {
    u8 *src_pixel = src->pixel(src_x, src_y);
    u8 *dst_pixel = dst->pixel(dst_x, dst_y);
    dst_pixel[0] = src_pixel[0];
    dst_pixel[1] = src_pixel[1];
}

static void copy_heightmap_column(Ref<Image> src, u32 src_x, u32 src_y,
                                  Ref<Image> dst, u32 dst_x, u32 dst_y,
                                  u32 count) {
    if (!valid_heightmap(src) || !valid_heightmap(dst)) return;
    for (u32 i = 0; i < count; i++) {
        copy_height_pixel(src, src_x, src_y + i, dst, dst_x, dst_y + i);
    }
}

static void copy_heightmap_row(Ref<Image> src, u32 src_x, u32 src_y,
                               Ref<Image> dst, u32 dst_x, u32 dst_y,
                               u32 count) {
    if (!valid_heightmap(src) || !valid_heightmap(dst)) return;
    for (u32 i = 0; i < count; i++) {
        copy_height_pixel(src, src_x + i, src_y, dst, dst_x + i, dst_y);
    }
}

static void clamp_heightmap_border(Ref<Image> heightmap) {
    if (!valid_heightmap(heightmap)) return;

    for (u32 y = EDITOR_HEIGHTMAP_INNER_FIRST;
         y <= EDITOR_HEIGHTMAP_INNER_LAST; y++) {
        copy_height_pixel(heightmap, EDITOR_HEIGHTMAP_INNER_FIRST, y,
                          heightmap, EDITOR_HEIGHTMAP_GHOST_FIRST, y);
        copy_height_pixel(heightmap, EDITOR_HEIGHTMAP_INNER_LAST, y, heightmap,
                          EDITOR_HEIGHTMAP_GHOST_LAST, y);
    }

    for (u32 x = EDITOR_HEIGHTMAP_GHOST_FIRST;
         x <= EDITOR_HEIGHTMAP_GHOST_LAST; x++) {
        copy_height_pixel(heightmap, x, EDITOR_HEIGHTMAP_INNER_FIRST,
                          heightmap, x, EDITOR_HEIGHTMAP_GHOST_FIRST);
        copy_height_pixel(heightmap, x, EDITOR_HEIGHTMAP_INNER_LAST, heightmap,
                          x, EDITOR_HEIGHTMAP_GHOST_LAST);
    }
}

static void rebuild_heightmap_border(const std::vector<EditorChunk> &chunks,
                                     std::vector<Ref<Image>> &heightmaps,
                                     u32 chunk_index) {
    if (chunk_index >= chunks.size() || chunk_index >= heightmaps.size() ||
        !valid_heightmap(heightmaps[chunk_index])) {
        return;
    }

    const EditorChunk &chunk = chunks[chunk_index];
    Ref<Image> heightmap = heightmaps[chunk_index];
    clamp_heightmap_border(heightmap);

    i32 left = find_chunk_index_at(chunks, chunk.x - 1, chunk.y);
    if (left >= 0 && (u32)left < heightmaps.size()) {
        copy_heightmap_column(heightmaps[left], EDITOR_HEIGHTMAP_INNER_LAST - 1,
                              EDITOR_HEIGHTMAP_INNER_FIRST, heightmap,
                              EDITOR_HEIGHTMAP_GHOST_FIRST,
                              EDITOR_HEIGHTMAP_INNER_FIRST,
                              EDITOR_HEIGHTMAP_INNER_SIZE);
    }

    i32 right = find_chunk_index_at(chunks, chunk.x + 1, chunk.y);
    if (right >= 0 && (u32)right < heightmaps.size()) {
        copy_heightmap_column(heightmaps[right],
                              EDITOR_HEIGHTMAP_INNER_FIRST + 1,
                              EDITOR_HEIGHTMAP_INNER_FIRST, heightmap,
                              EDITOR_HEIGHTMAP_GHOST_LAST,
                              EDITOR_HEIGHTMAP_INNER_FIRST,
                              EDITOR_HEIGHTMAP_INNER_SIZE);
    }

    i32 bottom = find_chunk_index_at(chunks, chunk.x, chunk.y - 1);
    if (bottom >= 0 && (u32)bottom < heightmaps.size()) {
        copy_heightmap_row(heightmaps[bottom], EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_INNER_LAST - 1, heightmap,
                           EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_GHOST_FIRST,
                           EDITOR_HEIGHTMAP_INNER_SIZE);
    }

    i32 top = find_chunk_index_at(chunks, chunk.x, chunk.y + 1);
    if (top >= 0 && (u32)top < heightmaps.size()) {
        copy_heightmap_row(heightmaps[top], EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_INNER_FIRST + 1, heightmap,
                           EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_GHOST_LAST,
                           EDITOR_HEIGHTMAP_INNER_SIZE);
    }
}

static void copy_neighbor_edges_to_new_chunk(
    const std::vector<EditorChunk> &chunks,
    const std::vector<Ref<Image>> &heightmaps, i32 x, i32 y,
    Ref<Image> heightmap) {
    i32 left = find_chunk_index_at(chunks, x - 1, y);
    if (left >= 0 && (u32)left < heightmaps.size()) {
        copy_heightmap_column(heightmaps[left], EDITOR_HEIGHTMAP_INNER_LAST,
                              EDITOR_HEIGHTMAP_INNER_FIRST, heightmap,
                              EDITOR_HEIGHTMAP_INNER_FIRST,
                              EDITOR_HEIGHTMAP_INNER_FIRST,
                              EDITOR_HEIGHTMAP_INNER_SIZE);
    }

    i32 right = find_chunk_index_at(chunks, x + 1, y);
    if (right >= 0 && (u32)right < heightmaps.size()) {
        copy_heightmap_column(heightmaps[right], EDITOR_HEIGHTMAP_INNER_FIRST,
                              EDITOR_HEIGHTMAP_INNER_FIRST, heightmap,
                              EDITOR_HEIGHTMAP_INNER_LAST,
                              EDITOR_HEIGHTMAP_INNER_FIRST,
                              EDITOR_HEIGHTMAP_INNER_SIZE);
    }

    i32 bottom = find_chunk_index_at(chunks, x, y - 1);
    if (bottom >= 0 && (u32)bottom < heightmaps.size()) {
        copy_heightmap_row(heightmaps[bottom], EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_INNER_LAST, heightmap,
                           EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_INNER_SIZE);
    }

    i32 top = find_chunk_index_at(chunks, x, y + 1);
    if (top >= 0 && (u32)top < heightmaps.size()) {
        copy_heightmap_row(heightmaps[top], EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_INNER_FIRST, heightmap,
                           EDITOR_HEIGHTMAP_INNER_FIRST,
                           EDITOR_HEIGHTMAP_INNER_LAST,
                           EDITOR_HEIGHTMAP_INNER_SIZE);
    }
}

static void sync_loaded_heightmap_seams(std::vector<EditorChunk> &chunks,
                                        std::vector<Ref<Image>> &heightmaps) {
    for (u32 i = 0; i < chunks.size(); i++) {
        if (i >= heightmaps.size() || !valid_heightmap(heightmaps[i])) {
            continue;
        }

        EditorChunk &chunk = chunks[i];
        i32 left = find_chunk_index_at(chunks, chunk.x - 1, chunk.y);
        if (left >= 0 && (u32)left < heightmaps.size()) {
            copy_heightmap_column(heightmaps[i], EDITOR_HEIGHTMAP_INNER_FIRST,
                                  EDITOR_HEIGHTMAP_INNER_FIRST,
                                  heightmaps[left],
                                  EDITOR_HEIGHTMAP_INNER_LAST,
                                  EDITOR_HEIGHTMAP_INNER_FIRST,
                                  EDITOR_HEIGHTMAP_INNER_SIZE);
        }

        i32 bottom = find_chunk_index_at(chunks, chunk.x, chunk.y - 1);
        if (bottom >= 0 && (u32)bottom < heightmaps.size()) {
            copy_heightmap_row(heightmaps[i], EDITOR_HEIGHTMAP_INNER_FIRST,
                               EDITOR_HEIGHTMAP_INNER_FIRST,
                               heightmaps[bottom],
                               EDITOR_HEIGHTMAP_INNER_FIRST,
                               EDITOR_HEIGHTMAP_INNER_LAST,
                               EDITOR_HEIGHTMAP_INNER_SIZE);
        }
    }

    for (u32 i = 0; i < chunks.size(); i++) {
        rebuild_heightmap_border(chunks, heightmaps, i);
    }
}

static Json write_sky(const SkySetting &sky) {
    Json j = Json::object();
    j["up"] = sky.up;
    j["down"] = sky.down;
    j["left"] = sky.left;
    j["right"] = sky.right;
    j["front"] = sky.front;
    j["back"] = sky.back;
    return j;
}

static Json write_directional_light(const DirectionalLightSetting &light) {
    Json j = Json::object();
    j["direction"] = light.direction;
    j["diffuse"] = light.diffuse;
    j["specular"] = light.specular;
    return j;
}

static Json write_point_light(const PointLightSetting &light) {
    Json j = Json::object();
    j["position"] = light.position;
    j["diffuse"] = light.diffuse;
    j["specular"] = light.specular;
    return j;
}

static Json write_static_object(const StaticObjectSetting &object) {
    Json j = Json::object();
    j["name"] = object.name;
    j["x"] = object.x;
    j["y"] = object.y;
    j["model"] = object.model;
    return j;
}

static Json write_chunk(const ChunkSetting &chunk) {
    Json j = Json::object();
    j["x"] = chunk.x;
    j["y"] = chunk.y;
    j["height_map"] = chunk.height_map;

    j["position_lights"] = Json::array();
    for (const PointLightSetting &light : chunk.lights) {
        j["position_lights"].push_back(write_point_light(light));
    }

    j["static_objects"] = Json::array();
    for (const StaticObjectSetting &object : chunk.static_objects) {
        j["static_objects"].push_back(write_static_object(object));
    }

    return j;
}

EditorWorld::EditorWorld(ResourceEntry *entry) : entry(entry) {
    if (entry != nullptr) {
        config = &entry->config;
    }
    setting.create();
    terrain.create();
    reload();
}

void EditorWorld::add_new_chunk(i32 x, i32 y) {
    ResourceEntry *entry = gEditor->create_internal_asset(
        fmt::format("{}_{}_{}.png", setting->name, x, y), type_id<Texture>());
    Ref<Image> heightmap;
    heightmap.create(PixelFormat::RG, EDITOR_HEIGHTMAP_SIZE,
                     EDITOR_HEIGHTMAP_SIZE);
    heightmap->fill(Color{64, 64}, EDITOR_HEIGHTMAP_SIZE,
                    EDITOR_HEIGHTMAP_SIZE);
    copy_neighbor_edges_to_new_chunk(setting->chunks, heightmaps, x, y,
                                     heightmap);

    EditorChunk chunk;
    chunk.x = x;
    chunk.y = y;
    chunk.height_map = entry->uuid;
    setting->chunks.push_back(chunk);
    heightmaps.push_back(heightmap);
    sync_loaded_heightmap_seams(setting->chunks, heightmaps);

    terrain->add_chunk(chunk.x, chunk.y, heightmap);
    heightmap->save_disk(entry->real_path());
}

void EditorWorld::clear_tiles() {
    setting->chunks.clear();
    heightmaps.clear();
    terrain->clear_chunks();
}

void EditorWorld::reload() {
    sky = {};
    heightmaps.clear();
    terrain->clear_chunks();
    if (entry == nullptr) return;

    Ref<WorldSetting> loaded =
        ResourceLoader::get_instance()->load<WorldSetting>(entry->uuid);
    if (!loaded.is_null()) {
        setting = loaded;
    }

    copy_sky_setting_to_editor(setting->sky, sky);
    apply_directional_light_to_runtime();

    for (EditorChunk &chunk : setting->chunks) {
        Ref<Image> heightmap =
            ResourceLoader::get_instance()->load<Image>(chunk.height_map);
        heightmaps.push_back(heightmap);
    }

    sync_loaded_heightmap_seams(setting->chunks, heightmaps);
    for (u32 i = 0; i < setting->chunks.size(); i++) {
        if (i < heightmaps.size() && !heightmaps[i].is_null()) {
            terrain->add_chunk(setting->chunks[i].x, setting->chunks[i].y,
                               heightmaps[i]);
        }
    }
}

void EditorWorld::save() {
    if (config == nullptr || setting.is_null()) return;
    copy_editor_sky_to_setting(sky, setting->sky);

    Json &j = config->get_json();
    if (!j.is_object()) j = Json::object();
    j["name"] = setting->name;
    j["sky"] = write_sky(setting->sky);
    j["directional_light"] = write_directional_light(setting->dir_light);

    j["chunks"] = Json::array();
    for (const EditorChunk &chunk : setting->chunks) {
        j["chunks"].push_back(write_chunk(chunk));
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

bool EditorWorldInspector::draw_vec3(KStr label, Vec3 &value) {
    ImGui::PushID(label.data());
    ImGui::TextUnformatted(label.data(), label.end());
    bool changed = ImGui::DragFloat3("##value", value.coord, 0.05f);
    ImGui::PopID();
    return changed;
}

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
        u32 width = sky.cubemap->get_width();
        u32 height = sky.cubemap->get_height();
        auto update_skybox = [&](UUID uuid, CubemapFace face) {
            auto image = ResourceLoader::get_instance()->load<Image>(uuid);
            sky.cubemap->update_face(width, height, face, image->get_data());
        };

        if (drag_uuid("up", sky.up)) update_skybox(sky.up, CubemapFace::TOP);
        if (drag_uuid("down", sky.down))
            update_skybox(sky.down, CubemapFace::BOTTOM);
        if (drag_uuid("left", sky.left))
            update_skybox(sky.left, CubemapFace::LEFT);
        if (drag_uuid("right", sky.right))
            update_skybox(sky.right, CubemapFace::RIGHT);
        if (drag_uuid("front", sky.front))
            update_skybox(sky.front, CubemapFace::FRONT);
        if (drag_uuid("back", sky.back))
            update_skybox(sky.back, CubemapFace::BACK);
    }

    if (ImGui::CollapsingHeader("Directional Light",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        EditorDirectionalLight &light = world->get_directional_light();
        bool changed = draw_vec3("direction", light.direction);
        changed |= draw_vec3("diffuse", light.diffuse);
        changed |= draw_vec3("specular", light.specular);
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

}  // namespace Seed
