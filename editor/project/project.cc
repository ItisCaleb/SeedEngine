#include "project.h"
#include "core/io/file.h"

namespace Seed {
Project *Project::load(const std::string &path) {
    auto file = File::open(path);
    auto json = file->read_json();
    Project *project = new Project;
    project->name = json["name"];
    project->path = path;
    return project;
}
void Project::save() {
    auto file = File::open(path + "/" + this->name + ".json", "wb");
    nlohmann::json j;
    j["name"] = this->name;
    std::string json_s = j.dump();
    file->write(json_s.data(), json_s.size());
}

}  // namespace Seed