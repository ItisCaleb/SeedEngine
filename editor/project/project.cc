#include "project.h"

#include <queue>

#include "core/io/dir.h"
#include "core/io/file.h"
#include "core/io/path.h"
#include "core/misc/type_name.h"
#include "core/resource/resource_entry.h"
#include "core/resource/texture.h"
#include "core/resource/world_setting.h"
#include "core/system.h"

namespace Seed {

Ref<Project> Project::load(const Path &path) {
    Ref<File> file = File::open(path);
    if (file.is_null()) return {};

    nlohmann::json json = file->read_json();
    Ref<Project> project;
    project.create();
    project->name = json["name"];
    project->path = file->get_fullpath().parent();
    project->asset_dir = project->path.append("assets");
    project->internal_dir = project->asset_dir.append(".internal");
    project->entry_path = project->asset_dir.append(".seed_entry");
    project->preprocess_entry_path =
        project->asset_dir.append(".preprocess_entry");
    Dir::create_if_not_exists(project->asset_dir);
    Dir::create_if_not_exists(project->internal_dir);

    ResourceEntries *entries = System::gResourceEntries;
    entries->set_root(project->path);
    if (project->entry_path.is_file())
        entries->load(project->entry_path);
    else
        entries->save(project->entry_path);

    if (project->preprocess_entry_path.is_file()) {
        project->preprocessor.get_entries().load(
            project->preprocess_entry_path);
    }
    return project;
}

Path Project::resolve_asset(const Path &path) const {
    if (path.is_absolute()) return path;
    return this->path.append(path.to_str());
}

ResourceTypeID Project::extension_to_type(KStr extension) const {
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
        extension == ".tga" || extension == ".exr" || extension == ".hdr") {
        return type_id<Texture>();
    }
    if (extension == ".world") return type_id<WorldSetting>();
    return 0;
}

void Project::scan_assets() {
    Ref<Dir> root = Dir::open(asset_dir);
    if (root.is_null()) return;

    std::queue<Ref<Dir>> directories;
    directories.push(root);
    while (!directories.empty()) {
        Ref<Dir> directory = directories.front();
        directories.pop();
        for (Path child : directory->list()) {
            child = directory->concat(child);
            if (child.is_directory()) {
                directories.push(Dir::open(child));
                continue;
            }

            Path relative_path = child.relative(path);
            if (!System::gResourceEntries->get_uuid(relative_path).is_null()) {
                continue;
            }
            ResourceTypeID type_id =
                extension_to_type(relative_path.extension());
            if (type_id != 0) {
                System::gResourceEntries->insert_entry(relative_path, type_id);
            }
        }
    }
    save();
}

ResourceEntry *Project::create_asset(const Path &asset_path,
                                     ResourceTypeID type_id) {
    Path relative_path = asset_path;
    if (relative_path.is_absolute())
        relative_path = relative_path.relative(path);

    UUID uuid = System::gResourceEntries->insert_entry(relative_path, type_id);
    if (uuid.is_null()) return nullptr;
    return System::gResourceEntries->get_entry(uuid);
}

ResourceEntry *Project::create_internal_asset(KStr name,
                                              ResourceTypeID type_id) {
    Path asset_path = "assets/.internal";
    asset_path.push(name);
    return create_asset(asset_path, type_id);
}

void Project::remove_asset(UUID uuid) {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    if (entry == nullptr) return;
    File::remove(entry->real_path());
    System::gResourceEntries->remove_entry(uuid);
}

void Project::import_asset(const Path &origin_path, const Path &target_dir) {
    Path moved_path = target_dir.append(origin_path.filename());
    Ref<File> origin = File::open(origin_path);
    if (origin.is_null()) return;
    origin->copy_to(resolve_asset(moved_path));

    if (!preprocessor.try_preprocess(origin, moved_path)) {
        ResourceTypeID type_id = extension_to_type(origin_path.extension());
        if (type_id == 0) return;
        System::gResourceEntries->insert_entry(moved_path, type_id);
    }
    save();
}

PreprocessEntry *Project::get_preprocessed_entry(const Path &path) {
    return preprocessor.get_entry_from_path(path);
}

void Project::save() {
    System::gResourceEntries->save(get_entry_path());
    preprocessor.get_entries().save(get_preprocess_entry_path());
}

}  // namespace Seed
