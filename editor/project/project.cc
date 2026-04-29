#include "project.h"
#include <fmt/format.h>
#include <queue>
#include "core/concurrency/thread_pool.h"
#include "core/io/file.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/resource/resource.h"
#include "core/resource/resource_loader.h"
#include "core/serialize/json_impl.h"
#include "core/world/world.h"
#include "editor/editor.h"

namespace Seed {

Project *Project::load(const std::string &path) {
    auto file = File::open(path);
    if (file.is_null()) return nullptr;
    auto json = file->read_json();
    Project *project = new Project;
    project->name = json["name"];
    project->path = file->get_fullpath().parent();
    project->asset_dir = project->path.append("assets");
    project->internal_dir = project->asset_dir.append(".internal");
    project->entry_path = project->asset_dir.append(".seed_entry");
    project->preprocess_entry_path =
        project->asset_dir.append(".preprocess_entry");
    Dir::create_if_not_exists(project->asset_dir);
    if (!File::exists(project->entry_path)) {
        ResourceLoader::get_instance()->get_entries().save(project->entry_path);
    } else {
        ResourceLoader::get_instance()->get_entries().load(project->entry_path);
    }
    // ThreadPool::get_instance()->add_work(
    //     [=](void *) {});
    project->scan_assets();
    return project;
}

ResourceTypeID Project::extension_to_tid(KStr ext) {
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
        ext == ".exr" || ext == ".hdr")
        return type_id<Texture>();
    if (ext == ".world") return type_id<World>();
    // if (ext == ".terrain")  // adjust to your format
    //     return type_id<Terrain>();
    return 0;
}

void Project::scan_assets() {
    Ref<Dir> dir = Dir::open(get_asset_dir());
    std::queue<Ref<Dir>> dirs_to_process;
    dirs_to_process.push(dir);
    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    while (!dirs_to_process.empty()) {
        Ref<Dir> d = dirs_to_process.front();
        dirs_to_process.pop();
        std::vector<Path> childs = d->list();
        for (Path &path : childs) {
            path = d->concat(path);

            if (path.is_directory()) {
                dirs_to_process.push(Dir::open(path));
            } else {
                path = path.relative(get_path());
                if (!entries.get_uuid(path).is_null()) continue;
                KStr extension = path.extension();
                u64 tid = extension_to_tid(extension);
                if (tid == 0) continue;
                entries.insert_entry(path, tid);
            }
        }
    }
    entries.save(get_entry_path());
}
UUID Project::create_asset(const Path &path, ResourceTypeID tid) {
    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    UUID uuid = entries.insert_entry(path, tid);
    entries.save(get_entry_path());
    return uuid;
}

void Project::import_asset(const Path &origin_path, const Path &target_dir) {
    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    Path moved_path = target_dir.append(origin_path.filename());
    Ref<File> origin = File::open(origin_path);
    origin->copy_to(get_path().append(moved_path));

    bool r = gEditor->preprocessor.try_preprocess(entries, origin, moved_path);
    if (r) {
        gEditor->preprocessor.get_entries().save(preprocess_entry_path);
    } else {
        KStr extension = origin_path.extension();
        u64 tid = extension_to_tid(extension);
        if (tid == 0) return;
        entries.insert_entry(moved_path, tid);
    }
    entries.save(get_entry_path());
}

void Project::save() {
    Path save_path = path;
    path.push(fmt::format("{}.json", this->name));
    auto file = File::open(save_path, "wb");
    nlohmann::json j;
    j["name"] = this->name;
    j["assets"] = assets;
    std::string json_s = j.dump();
    file->write(json_s.data(), json_s.size());
}

}  // namespace Seed