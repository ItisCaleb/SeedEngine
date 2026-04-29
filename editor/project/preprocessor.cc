#include "preprocessor.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/io/file.h"
#include "core/io/path.h"
#include "core/misc/type_name.h"
#include "core/misc/uuid.h"
#include "core/resource/model.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/serialize/json_impl.h"
#include "editor/asset/model_loader.h"
#include "editor/editor.h"

namespace Seed {

PreprocessEntry *PreprocessEntries::get_entry(const UUID uuid) {
    auto iter = uuid_to_entry.find(uuid);
    if (iter == uuid_to_entry.end()) {
        return nullptr;
    }
    return &iter->second;
}

UUID PreprocessEntries::get_uuid(const Path &path) {
    auto iter = path_to_uuid.find(path);
    if (iter == path_to_uuid.end()) {
        return UUID{};
    }
    return iter->second;
}
UUID PreprocessEntries::insert_entry(const Path &p, ResourceTypeID id) {
    auto iter = path_to_uuid.find(p);
    if (iter != path_to_uuid.end()) return iter->second;
    UUID uuid = UUID::generate();

    ResourceConfiguration config;
    this->uuid_to_entry[uuid] = PreprocessEntry{
        .uuid = uuid, .type_id = id, .path = p, .config = config};
    this->path_to_uuid[p] = uuid;

    return uuid;
}

void PreprocessEntries::link_entry(UUID from, UUID to) {
    this->uuid_to_entry[from].target_uuid = to;
}

void PreprocessEntries::remove_entry(const UUID uuid) {
    auto iter = uuid_to_entry.find(uuid);
    if (iter != uuid_to_entry.end()) {
        uuid_to_entry.erase(iter);
    }
}
void PreprocessEntries::save(const Path &path) {
    Ref<File> file = File::open(path, "wb");
    nlohmann::ordered_json j;
    j["entries"] = nlohmann::json::array();
    for (auto &[uuid, entry] : uuid_to_entry) {
        nlohmann::ordered_json j_entry;
        j_entry["UUID"] = uuid;
        j_entry["target_UUID"] = entry.target_uuid;
        j_entry["resource_type_id"] = entry.type_id;
        j_entry["path"] = entry.path;
        j_entry["config"] = entry.config.get_json();
        j["entries"].push_back(j_entry);
    }
    file->write_str(j.dump(2));
}
void PreprocessEntries::load(const Path &path) {
    Ref<File> file = File::open(path);
    nlohmann::json j = file->read_json();
    auto j_entries = j["entries"];
    ResourceLoader *loader = ResourceLoader::get_instance();
    for (auto j_entry : j_entries) {
        UUID uuid = j_entry["UUID"];
        ResourceTypeID type_id = j_entry["resource_type_id"];
        UUID target_uuid = j_entry["target_UUID"];
        Path p = j_entry["path"];
        nlohmann::json jconfig;
        ResourceTypeInfo *info = loader->get_type_info(type_id);
        if (info && info->has_data) {
            Ref<File> config = File::open(p);
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
        this->uuid_to_entry[uuid] = PreprocessEntry{.uuid = uuid,
                                                    .target_uuid = target_uuid,
                                                    .type_id = type_id,
                                                    .path = p,
                                                    .config = jconfig};
        this->path_to_uuid[p] = uuid;
    }
}

bool Preprocessor::preprocess(ResourceEntries &entries, Ref<File> file,
                              const Path &moved_path,
                              PreprocessTypeInfo &info) {
    Path &internal_dir = gEditor->project()->get_internal_dir();
    Dir::create_if_not_exists(internal_dir);

    UUID from_uuid = preprocess_entries.insert_entry(moved_path, info.id);
    ResourceConfiguration out_config;
    if (info.generate_config) {
        info.generate_config(out_config);
    }
    PreprocessEntry *entry = preprocess_entries.get_entry(from_uuid);
    PreprocessedResult result;
    bool r = info.preprocess(entry->config, file, out_config,
                             Dir::open(internal_dir), result);
    if (!r) {
        SPDLOG_ERROR("Preprocess failed!");
        return false;
    }
    Path out_path = internal_dir.append(result.out_file);
    UUID to_uuid = entries.insert_entry(
        out_path.relative(gEditor->project()->get_path()), result.target_tid);
    preprocess_entries.link_entry(from_uuid, to_uuid);
    entries.get_entry(to_uuid)->config = out_config;

    return true;
}

bool Preprocessor::try_preprocess(ResourceEntries &entries, Ref<File> file,
                                  const Path &moved_path) {
    KStr extension = file->get_path().extension();
    auto iter = ext_to_infos.find(extension);
    if (iter == ext_to_infos.end()) return false;
    return preprocess(entries, file, moved_path, iter->second);
}

Preprocessor::Preprocessor() {
    register_type<EditorModel>({".fbx", ".obj", ".gltf"}, process_model);
}

bool Preprocessor::process_model(ResourceConfiguration &in_conf,
                                 Ref<File> &in_data,
                                 ResourceConfiguration &out_conf,
                                 Ref<Dir> out_dir,
                                 PreprocessedResult &out_result) {
    EditorModel model(in_data->get_fullpath());
    out_result.out_file =
        fmt::format("{}.bin", in_data->get_filename_without_ext());
    model.dump(out_conf, out_dir->open_file(out_result.out_file, "wb"));
    if (!model.bones.empty()) {
        out_result.target_tid = type_id<SkeletonModel>();
    } else {
        out_result.target_tid = type_id<BasicModel>();
    }
    return true;
}

void Preprocessor::init(const Path &path) {}

}  // namespace Seed