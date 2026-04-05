#include "project.h"
#include <fmt/format.h>
#include "core/io/file.h"
#include "core/io/dir.h"
#include "core/io/path.h"

namespace Seed {
Project *Project::load(const std::string &path) {
    auto file = File::open(path);
    if (file.is_null()) return nullptr;
    auto json = file->read_json();
    Project *project = new Project;
    project->name = json["name"];
    project->path = file->get_directory();
    project->asset_dir = project->path;
    project->asset_dir.push("assets");
    Dir::create_if_not_exists(project->asset_dir);
    return project;
}
Path &Project::get_asset_dir() { return asset_dir; }

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