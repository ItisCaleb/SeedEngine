#include "project.h"
#include "core/io/file.h"
#include "core/io/dir.h"

namespace Seed {
Project *Project::load(const std::string &path) {
    auto file = File::open(path);
    if (file.is_null()) return nullptr;
    auto json = file->read_json();
    Project *project = new Project;
    project->name = json["name"];
    project->path = file->get_directory();
    Dir::create_if_not_exists(project->get_asset_dir());
    return project;
}
std::string Project::get_asset_dir() { return path + "/assets/"; }

void Project::save() {
    auto file = File::open(path + "/" + this->name + ".json", "wb");
    nlohmann::json j;
    j["name"] = this->name;
    std::string json_s = j.dump();
    file->write(json_s.data(), json_s.size());
}

}  // namespace Seed