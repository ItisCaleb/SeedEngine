#include "asset.h"
#include <imgui.h>
#include "core/container/kstring.h"
#include "core/misc/uuid.h"
#include "core/serialize/json_impl.h"
#include "editor/gui/editor_ui.h"

namespace Seed {
ModelInspector::ModelInspector(ResourceConfiguration &config)
    : Inspectable(&config) {
    auto &j = config.get_json();
    for (auto &mat_j : j["materials"]) {
        if (mat_j["name"] == "") continue;
        Material m;
        m.name = mat_j["name"];
        m.diffuse = mat_j.value("diffuse", UUID());
        m.specular = mat_j.value("specular", UUID());
        m.normal = mat_j.value("normal", UUID());
        m.opacity = mat_j.value("opacity", 1.0f);
        materials.push_back(m);
    }
}
void ModelInspector::draw_inspector() {
    EditorUI::section("Materials");
    if (materials.empty()) {
        ImGui::TextDisabled("No materials.");
        return;
    }

    for (auto &mat : materials) {
        ImGui::TextUnformatted(mat.name.data());
        drag_uuid("diffuse", mat.diffuse);
        drag_uuid("specular", mat.specular);
        drag_uuid("normal", mat.normal);
    }
}
void ModelInspector::save() {
    auto &j = config->get_json();
    j["materials"] = nlohmann::json::array();
    for (auto &mat : materials) {
        nlohmann::ordered_json mat_j;
        mat_j["name"] = mat.name;
        mat_j["diffuse"] = mat.diffuse;
        mat_j["specular"] = mat.specular;
        mat_j["normal"] = mat.normal;
        mat_j["opacity"] = mat.opacity;
        j["materials"].push_back(mat_j);
    }
}

}  // namespace Seed
