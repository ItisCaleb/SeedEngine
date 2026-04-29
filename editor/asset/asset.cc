#include "asset.h"
#include <fmt/base.h>
#include <imgui.h>
#include <nlohmann/json_fwd.hpp>
#include "core/container/kstring.h"
#include "core/misc/uuid.h"
#include "core/resource/resource_entry.h"
#include "core/serialize/json_impl.h"
#include <nfd.h>
#include "core/resource/resource_loader.h"
#include "editor/editor.h"
#include <stb_image.h>

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

void WorldCreatePopup::create_world(){
    Ref<Dir> current_dir = gEditor->get_current_dir();
    ResourceConfiguration config;
    nlohmann::ordered_json &j = config.get_json();
    Path name = current_dir->concat(KStr(new_terrain_name));
}

void WorldCreatePopup::draw() {
    if (!should_close) ImGui::OpenPopup("##new_terrain_modal");

    // Center the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_Appearing);

    bool open = true;
    if (ImGui::BeginPopupModal(
            "##new_terrain_modal", &open,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        // Title row
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.85f, 1.f, 1.f));
        ImGui::Text("New Terrain");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        // Name
        ImGui::Text("Name");
        ImGui::PushItemWidth(-1);
        ImGui::InputText("##nt_name", new_terrain_name, 64);
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // Dimensions
        ImGui::Text("Size");
        ImGui::BeginDisabled(load_from_heightmap);
        ImGui::PushItemWidth(100);
        ImGui::InputInt("W##nt_w", &new_terrain_w);
        ImGui::SameLine();
        ImGui::InputInt("H##nt_h", &new_terrain_h);
        ImGui::PopItemWidth();
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Heightmap drop zone
        ImGui::Text("Load Heightmap  (optional)");
        ImVec2 dz_pos = ImGui::GetCursorScreenPos();
        ImVec2 dz_size = ImVec2(ImGui::GetContentRegionAvail().x, 64);

        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(dz_pos,
                          ImVec2(dz_pos.x + dz_size.x, dz_pos.y + dz_size.y),
                          IM_COL32(20, 22, 26, 255), 4);
        dl->AddRect(dz_pos, ImVec2(dz_pos.x + dz_size.x, dz_pos.y + dz_size.y),
                    IM_COL32(80, 100, 130, 180), 4, 0, 1.f);

        // Invisible button to catch click
        ImGui::InvisibleButton("##dropzone", dz_size);
        bool dz_hovered = ImGui::IsItemHovered();

        const char *dz_label = "Drop  .png / .exr  or click to browse";
        ImVec2 ts = ImGui::CalcTextSize(dz_label);
        dl->AddText(ImVec2(dz_pos.x + (dz_size.x - ts.x) * 0.5f,
                           dz_pos.y + (dz_size.y - ts.y) * 0.5f),
                    dz_hovered ? IM_COL32(120, 180, 255, 255)
                               : IM_COL32(80, 90, 100, 255),
                    dz_label);
        ResourceLoader *loader = ResourceLoader::get_instance();

        if (ImGui::IsItemClicked()) {
            nfdu8char_t *path;
            nfdopendialogu8args_t args = {0};
            nfdresult_t r = NFD_OpenDialogU8_With(&path, &args);
            if (r == NFD_OKAY) {
                i32 x, y, comp;
                i32 result = stbi_info(path, &x, &y, &comp);
                if (result) {
                    new_terrain_w = align_to(x, 256);
                    new_terrain_h = align_to(y, 256);
                    height_map_path = KStr(path);
                    load_from_heightmap = true;
                }
            }
        }

        // Handle actual drag-drop from the OS via ImGui payload
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *p =
                    ImGui::AcceptDragDropPayload("EXTERNAL")) {
                KStr data(KStr((char *)p->Data, p->DataSize - 1));
                std::vector<KStr> files = data.split("\n");
                fmt::println("{}", files);
                i32 x, y, comp;
                i32 result = stbi_info(files[0].string().data(), &x, &y, &comp);
                if (result) {
                    new_terrain_w = align_to(x, 256);
                    new_terrain_h = align_to(y, 256);
                    load_from_heightmap = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Init options
        // ImGui::Checkbox("Initialize flat (height = 0)", &init_flat);
        // ImGui::Checkbox("Add base noise layer", &init_noise);

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
            // Allocate terrain
            // init_default_splat_layers();
            should_close = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);

        ImGui::EndPopup();
    }

    if (!open) should_close = false;
}

WorldInspector::WorldInspector(ResourceConfiguration &config)
    : Inspectable(&config) {
    auto &j = config.get_json();
    world = j;
}
void WorldInspector::draw_inspector() {
    ImGui::TextUnformatted("Sky");
    drag_uuid("up", world.sky.up);
    drag_uuid("down", world.sky.down);
    drag_uuid("left", world.sky.left);
    drag_uuid("right", world.sky.right);
    drag_uuid("front", world.sky.front);
    drag_uuid("back", world.sky.back);
    ImGui::Separator();
    ImGui::TextUnformatted("Textures");
    drag_uuid("texture1", world.texture1);
    drag_uuid("texture1_normal", world.texture1_normal);
}
void WorldInspector::save() {
    auto &j = config->get_json();
    j = world;
}
}  // namespace Seed