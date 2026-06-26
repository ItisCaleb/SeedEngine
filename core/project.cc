#include "project.h"

#include "core/container/kstring.h"
#include "core/io/file.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/resource/resource.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"

namespace Seed {


Project *Project::load(const Path &path) {
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

    return project;
}



const Path Project::resolve_asset(const Path &path){
    if(path.is_absolute()){
        return path;
    }else{
        return this->path.append(path);
    }
}

void Project::save() {
    // Path save_path = path;
    // path.push(fmt::format("{}.json", this->name));
    // auto file = File::open(save_path, "wb");
    // nlohmann::json j;
    // j["name"] = this->name;
    // std::string json_s = j.dump();
    // file->write(json_s.data(), json_s.size());
    ResourceLoader::get_instance()->get_entries().save(get_entry_path());
}

}  // namespace Seed