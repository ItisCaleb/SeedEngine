#include "editor_world.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <imgui.h>
#include "core/serialize/json_impl.h"

namespace Seed {

using Json = nlohmann::ordered_json;

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
    chunk.x = j.value("x", 0u);
    chunk.y = j.value("y", 0u);
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

void EditorWorld::reload() {
    name.clear();
    sky = {};
    directional_light = {};
    chunks.clear();
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

EditorWorldInspector::EditorWorldInspector(EditorWorld *world)
    : world(world) {}

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
        drag_uuid("up", sky.up);
        drag_uuid("down", sky.down);
        drag_uuid("left", sky.left);
        drag_uuid("right", sky.right);
        drag_uuid("front", sky.front);
        drag_uuid("back", sky.back);
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
    ImGui::Text("Chunks: %zu", world->get_chunks().size());
}

void EditorWorldInspector::save() {
    if (world != nullptr) world->save();
}

}  // namespace Seed
