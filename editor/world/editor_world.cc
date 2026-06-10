#include "editor_world.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <imgui.h>
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

static EditorSky read_sky(const Json &j) {
    EditorSky sky;
    if (!j.is_object()) return sky;
    sky.raw = j;
    sky.up = j.value("up", UUID{});
    if (sky.up.is_null()) sky.up = j.value("top", UUID{});
    sky.down = j.value("down", UUID{});
    if (sky.down.is_null()) sky.down = j.value("bottom", UUID{});
    sky.left = j.value("left", UUID{});
    sky.right = j.value("right", UUID{});
    sky.front = j.value("front", UUID{});
    sky.back = j.value("back", UUID{});
    sky.cubemap = ResourceLoader::get_instance()->load_cubemap(
        2048, 2048, sky.right, sky.left, sky.up, sky.down, sky.front, sky.back);
    sky.sky.create(sky.cubemap);
    return sky;
}

static EditorDirectionalLight read_directional_light(const Json &j) {
    EditorDirectionalLight light;
    if (!j.is_object()) return light;
    light.raw = j;
    light.direction = j.value("direction", light.direction);
    light.diffuse = j.value("diffuse", light.diffuse);
    light.specular = j.value("specular", light.specular);
    light.enabled = j.value("enabled", light.enabled);
    return light;
}

static EditorPointLight read_point_light(const Json &j) {
    EditorPointLight light;
    if (!j.is_object()) return light;
    light.raw = j;
    light.position = j.value("position", light.position);
    light.diffuse = j.value("diffuse", light.diffuse);
    light.specular = j.value("specular", light.specular);
    return light;
}

static EditorStaticObject read_static_object(const Json &j) {
    EditorStaticObject object;
    if (!j.is_object()) return object;
    object.raw = j;
    object.name = KString(j.value<std::string>("name", ""));
    object.x = j.value("x", 0);
    object.y = j.value("y", 0);
    object.model = j.value("model", UUID{});
    return object;
}

static EditorChunk read_chunk(const Json &j) {
    EditorChunk chunk;
    if (!j.is_object()) return chunk;
    chunk.raw = j;
    chunk.x = j.value("x", 0);
    chunk.y = j.value("y", 0);
    chunk.height_map = j.value("height_map", UUID{});

    if (j.contains("position_lights") && j["position_lights"].is_array()) {
        for (const Json &light_j : j["position_lights"]) {
            chunk.lights.push_back(read_point_light(light_j));
        }
    }
    if (j.contains("static_objects") && j["static_objects"].is_array()) {
        for (const Json &object_j : j["static_objects"]) {
            chunk.static_objects.push_back(read_static_object(object_j));
        }
    }
    return chunk;
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

static Json write_sky(EditorSky &sky) {
    Json j = sky.raw.is_object() ? sky.raw : Json::object();
    j["up"] = sky.up;
    j["down"] = sky.down;
    j["left"] = sky.left;
    j["right"] = sky.right;
    j["front"] = sky.front;
    j["back"] = sky.back;
    sky.raw = j;
    return j;
}

static Json write_directional_light(EditorDirectionalLight &light) {
    Json j = light.raw.is_object() ? light.raw : Json::object();
    j["direction"] = light.direction;
    j["diffuse"] = light.diffuse;
    j["specular"] = light.specular;
    j["enabled"] = light.enabled;
    light.raw = j;
    return j;
}

static Json write_point_light(EditorPointLight &light) {
    Json j = light.raw.is_object() ? light.raw : Json::object();
    j["position"] = light.position;
    j["diffuse"] = light.diffuse;
    j["specular"] = light.specular;
    light.raw = j;
    return j;
}

static Json write_static_object(EditorStaticObject &object) {
    Json j = object.raw.is_object() ? object.raw : Json::object();
    j["name"] = object.name;
    j["x"] = object.x;
    j["y"] = object.y;
    j["model"] = object.model;
    object.raw = j;
    return j;
}

static Json write_chunk(EditorChunk &chunk) {
    Json j = chunk.raw.is_object() ? chunk.raw : Json::object();
    j["x"] = chunk.x;
    j["y"] = chunk.y;
    j["height_map"] = chunk.height_map;

    j["position_lights"] = Json::array();
    for (EditorPointLight &light : chunk.lights) {
        j["position_lights"].push_back(write_point_light(light));
    }

    j["static_objects"] = Json::array();
    for (EditorStaticObject &object : chunk.static_objects) {
        j["static_objects"].push_back(write_static_object(object));
    }

    chunk.raw = j;
    return j;
}

EditorWorld::EditorWorld(ResourceConfiguration *config) : config(config) {
    terrain.create();
    reload();
}

void EditorWorld::add_new_chunk(i32 x, i32 y) {
    ResourceEntry *entry = gEditor->create_internal_asset(
        fmt::format("{}_{}_{}.png", name, x, y), type_id<Texture>());
    Ref<Image> heightmap;
    heightmap.create(PixelFormat::RG, EDITOR_HEIGHTMAP_SIZE,
                     EDITOR_HEIGHTMAP_SIZE);
    heightmap->fill(Color{64, 64}, EDITOR_HEIGHTMAP_SIZE,
                    EDITOR_HEIGHTMAP_SIZE);
    copy_neighbor_edges_to_new_chunk(chunks, heightmaps, x, y, heightmap);

    EditorChunk chunk;
    chunk.x = x;
    chunk.y = y;
    chunk.height_map = entry->uuid;
    chunks.push_back(chunk);
    heightmaps.push_back(heightmap);
    sync_loaded_heightmap_seams(chunks, heightmaps);

    terrain->add_chunk(chunk.x, chunk.y, heightmap);
    heightmap->save_disk(entry->real_path());
}

void EditorWorld::clear_tiles() {
    chunks.clear();
    heightmaps.clear();
    terrain->clear_chunks();
}

void EditorWorld::reload() {
    name.clear();
    sky = {};
    directional_light = {};
    chunks.clear();
    heightmaps.clear();
    terrain->clear_chunks();
    if (config == nullptr) return;

    Json &j = config->get_json();
    if (!j.is_object()) j = Json::object();
    name = KString(j.value<std::string>("name", ""));

    if (j.contains("sky")) sky = read_sky(j["sky"]);
    if (j.contains("directional_light")) {
        directional_light = read_directional_light(j["directional_light"]);
    }

    if (j.contains("chunks") && j["chunks"].is_array()) {
        for (const Json &chunk_j : j["chunks"]) {
            chunks.push_back(read_chunk(chunk_j));
            EditorChunk &chunk = chunks.back();
            Ref<Image> heightmap =
                ResourceLoader::get_instance()->load<Image>(chunk.height_map);
            heightmaps.push_back(heightmap);
        }
    }

    sync_loaded_heightmap_seams(chunks, heightmaps);
    for (u32 i = 0; i < chunks.size(); i++) {
        if (i < heightmaps.size() && !heightmaps[i].is_null()) {
            terrain->add_chunk(chunks[i].x, chunks[i].y, heightmaps[i]);
        }
    }
}

void EditorWorld::save() {
    if (config == nullptr) return;

    Json &j = config->get_json();
    if (!j.is_object()) j = Json::object();
    j["name"] = name;
    j["sky"] = write_sky(sky);
    j["directional_light"] = write_directional_light(directional_light);

    j["chunks"] = Json::array();
    for (EditorChunk &chunk : chunks) {
        j["chunks"].push_back(write_chunk(chunk));
    }
}

EditorWorldInspector::EditorWorldInspector(EditorWorld *world) : world(world) {}

void EditorWorldInspector::draw_vec3(KStr label, Vec3 &value) {
    ImGui::PushID(label.data());
    ImGui::TextUnformatted(label.data(), label.end());
    ImGui::DragFloat3("##value", value.coord, 0.05f);
    ImGui::PopID();
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
        ImGui::Checkbox("Enabled", &light.enabled);
        draw_vec3("direction", light.direction);
        draw_vec3("diffuse", light.diffuse);
        draw_vec3("specular", light.specular);
    }

    ImGui::Separator();
    ImGui::Text("Terrain tiles: %zu", world->get_chunks().size());
}

void EditorWorldInspector::save() {
    if (world != nullptr) world->save();
}

}  // namespace Seed
