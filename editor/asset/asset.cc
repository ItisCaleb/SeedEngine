#include "asset.h"
#include <fmt/base.h>
#include <fmt/format.h>
#include <imgui.h>
#include <nlohmann/json_fwd.hpp>
#include "core/container/kstring.h"
#include "core/misc/type_name.h"
#include "core/misc/uuid.h"
#include "core/serialize/json_impl.h"
#include "core/world/world.h"
#include "editor/editor.h"
#include <spdlog/spdlog.h>

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
    ImGui::TextUnformatted("Materials");
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

void WorldCreatePopup::create_world() {
    ResourceEntry *entry = gEditor->create_asset(
        fmt::format("{}.world", new_world_name), type_id<World>());
    nlohmann::ordered_json &j = entry->config.get_json();
    j["name"] = new_world_name;
    j["sky"] = {
        {"up", UUID{}},    {"down", UUID{}},  {"left", UUID{}},
        {"right", UUID{}}, {"front", UUID{}}, {"back", UUID{}},
    };
    j["directional_light"] = {
        {"enabled", true},
        {"direction", Vec3{-0.5f, -0.5f, 0.0f}},
        {"diffuse", Vec3{0.8f, 0.8f, 0.8f}},
        {"specular", Vec3{0.4f, 0.4f, 0.4f}},
    };
    j["chunks"] = nlohmann::ordered_json::array();
    spdlog::info("Saving world {}", new_world_name);
    gEditor->save_project();
    gEditor->world_editor.load_world(entry->path);
}

void WorldCreatePopup::draw() {
    if (!should_close) ImGui::OpenPopup("##new_world_modal");

    // Center the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_Appearing);

    bool open = true;
    if (ImGui::BeginPopupModal(
            "##new_world_modal", &open,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        // Title row
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.85f, 1.f, 1.f));
        ImGui::Text("New World");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        // Name
        ImGui::Text("Name");
        ImGui::PushItemWidth(-1);
        ImGui::InputText("##nt_name", new_world_name, 64);
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Spacing();
        ImGui::Spacing();

        // Footer buttons — right-aligned
        float btn_w = 90.f;
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - btn_w * 2 + 24);

        if (ImGui::Button("Cancel", ImVec2(btn_w, 0))) {
            should_close = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(0, 8);

        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.15f, 0.35f, 0.60f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.20f, 0.42f, 0.72f, 1.f));
        if (ImGui::Button("Create", ImVec2(btn_w, 0))) {
            create_world();
            should_close = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);

        ImGui::EndPopup();
    }

    if (!open) should_close = false;
}


}  // namespace Seed
