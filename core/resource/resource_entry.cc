#include "resource_entry.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json_fwd.hpp>
#include "core/engine.h"
#include "core/io/file.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/project.h"
#include "core/serialize/json_impl.h"
#include "core/resource/resource.h"
#include "core/resource/resource_loader.h"

namespace Seed {
ResourceEntry *ResourceEntries::get_entry(const UUID uuid) {
    auto iter = uuid_to_entry.find(uuid);
    if (iter == uuid_to_entry.end()) {
        return nullptr;
    }
    return &iter->second;
}

UUID ResourceEntries::get_uuid(const Path &path) {
    auto iter = path_to_uuid.find(path);
    if (iter == path_to_uuid.end()) {
        return UUID{};
    }
    return iter->second;
}
UUID ResourceEntries::insert_entry(const Path &p, ResourceTypeID id,
                                   bool is_internal) {
    ResourceTypeInfo *info = ResourceLoader::get_instance()->get_type_info(id);
    if (!info) {
        SPDLOG_ERROR(
            "Can't create entry with a type that doesn't registered yet!");
        return UUID{};
    }
    auto iter = path_to_uuid.find(p);
    if (iter != path_to_uuid.end()) return iter->second;
    UUID uuid = UUID::generate();

    ResourceConfiguration config;

    if (info && info->generate_config) {
        info->generate_config(config);
    }
    this->uuid_to_entry[uuid] =
        ResourceEntry{.uuid = uuid, .type_id = id, .path = p, .config = config};
    this->path_to_uuid[p] = uuid;
    if (is_internal) {
        this->internal_entries.insert(uuid);
    }

    return uuid;
}
void ResourceEntries::remove_entry(const UUID uuid) {
    auto iter = uuid_to_entry.find(uuid);
    if (iter != uuid_to_entry.end()) {
        uuid_to_entry.erase(iter);
    }
}
void ResourceEntries::save(const Path &path) {
    Ref<File> file = File::open(path, "wb");
    nlohmann::ordered_json j;
    j["entries"] = nlohmann::json::array();
    ResourceLoader *loader = ResourceLoader::get_instance();
    Project *project = SeedEngine::get_instance()->get_project();
    for (auto &[uuid, entry] : uuid_to_entry) {
        if (internal_entries.count(uuid) > 0) continue;
        ResourceTypeInfo *info = loader->get_type_info(entry.type_id);
        nlohmann::ordered_json j_entry;
        j_entry["UUID"] = uuid;
        j_entry["resource_type_id"] = entry.type_id;
        j_entry["path"] = entry.path;
        if (!info->has_data) {
            Ref<File> config =
                File::open(project->resolve_asset(entry.path), "wb");
            config->write_str(entry.config.get_json().dump(2));
        } else {
            j_entry["config"] = entry.config.get_json();
        }
        j["entries"].push_back(j_entry);
    }
    file->write_str(j.dump(2));
}
void ResourceEntries::load(const Path &path) {
    Ref<File> file = File::open(path);
    nlohmann::json j = file->read_json();
    auto &j_entries = j["entries"];
    ResourceLoader *loader = ResourceLoader::get_instance();
    Project *project = SeedEngine::get_instance()->get_project();

    for (auto &j_entry : j_entries) {
        UUID uuid = j_entry["UUID"];
        ResourceTypeID type_id = j_entry["resource_type_id"];
        Path p = j_entry["path"];
        nlohmann::json jconfig;
        ResourceTypeInfo *info = loader->get_type_info(type_id);
        if (info && !info->has_data) {
            Ref<File> config = File::open(project->resolve_asset(p));
            if (config.is_null()) {
                SPDLOG_ERROR(
                    "Resource entry loading error: Missing property of '{}'",
                    p);
                continue;
            }
            jconfig = config->read_json();
        } else {
            jconfig = j_entry["config"];
        }
        this->uuid_to_entry[uuid] = ResourceEntry{
            .uuid = uuid, .type_id = type_id, .path = p, .config = jconfig};
        this->path_to_uuid[p] = uuid;
    }
}

}  // namespace Seed