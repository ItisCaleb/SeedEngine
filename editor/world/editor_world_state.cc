#include "editor_world_state.h"

#include <utility>

#include <nlohmann/json.hpp>

#include "core/engine.h"
#include "core/resource/image.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/serialize/json_impl.h"
#include "core/system.h"
#include "core/world/world.h"

namespace Seed {

Ref<WorldSetting> EditorWorldState::load() const {
    Ref<WorldSetting> setting;
    if (entry != nullptr && System::gResourceLoader != nullptr) {
        setting = System::gResourceLoader->load<WorldSetting>(entry->uuid);
    }
    if (setting.is_null()) setting.create();
    return setting;
}

void EditorWorldState::save(const WorldSetting &setting) const {
    if (entry == nullptr) return;

    nlohmann::ordered_json &json = entry->config.get_json();
    if (!json.is_object()) json = nlohmann::ordered_json::object();

    json["name"] = setting.name;
    json["sky"] = {{"up", setting.sky.up},       {"down", setting.sky.down},
                   {"left", setting.sky.left},   {"right", setting.sky.right},
                   {"front", setting.sky.front}, {"back", setting.sky.back}};
    json["directional_light"] = {{"direction", setting.dir_light.direction},
                                 {"diffuse", setting.dir_light.diffuse},
                                 {"specular", setting.dir_light.specular}};
    json["terrain_textures"] = setting.terrain_textures;
    json["terrain_normals"] = setting.terrain_normals;
    json["chunks"] = nlohmann::ordered_json::array();

    for (const ChunkSetting &chunk : setting.chunks) {
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

void EditorWorldState::load_sky(const SkySetting &setting) {
    sky = {};
    sky.up = setting.up;
    sky.down = setting.down;
    sky.left = setting.left;
    sky.right = setting.right;
    sky.front = setting.front;
    sky.back = setting.back;

    if (System::gResourceLoader == nullptr) return;
    sky.cubemap = System::gResourceLoader->load_cubemap(
        2048, 2048, sky.right, sky.left, sky.up, sky.down, sky.front, sky.back);
    if (!sky.cubemap.is_null()) sky.sky.create(sky.cubemap);
}

void EditorWorldState::write_sky_setting(SkySetting &setting) const {
    setting.up = sky.up;
    setting.down = sky.down;
    setting.left = sky.left;
    setting.right = sky.right;
    setting.front = sky.front;
    setting.back = sky.back;
}

void EditorWorldState::update_skybox_face(UUID uuid, CubemapFace face) {
    if (uuid.is_null() || sky.cubemap.is_null() ||
        System::gResourceLoader == nullptr) {
        return;
    }

    Ref<Image> image = System::gResourceLoader->load_image(uuid, true);
    if (image.is_null()) return;
    sky.cubemap->update_face(sky.cubemap->get_width(),
                             sky.cubemap->get_height(), face, image);
}

void EditorWorldState::apply_directional_light(
    const DirectionalLightSetting &setting) const {
    if (System::gEngine == nullptr || System::gEngine->get_world() == nullptr) {
        return;
    }

    System::gEngine->get_world()->get_direction_light() = DirectionalLight(
        setting.direction, setting.diffuse, setting.specular, true);
}

void EditorWorldState::rebuild_static_models(
    const std::vector<ChunkSetting> &chunks) {
    static_models.clear();
    if (System::gResourceLoader == nullptr) return;

    for (const ChunkSetting &chunk : chunks) {
        for (const StaticObjectSetting &object : chunk.static_objects) {
            if (object.model.is_null()) continue;

            auto iter = static_models.find(object.model);
            if (iter == static_models.end()) {
                EditorStaticModel static_model;
                static_model.model =
                    System::gResourceLoader->load<BasicModel>(object.model);
                if (static_model.model.is_null()) continue;

                static_model.instances = static_model.model->create_instance();
                iter =
                    static_models.emplace(object.model, std::move(static_model))
                        .first;
            }

            Transform transform;
            transform.set_position((f32)object.x, (f32)object.y, (f32)object.z);
            iter->second.model->add_instance(iter->second.instances, transform);
        }
    }
}

}  // namespace Seed
