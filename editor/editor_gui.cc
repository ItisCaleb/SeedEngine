#include "editor_gui.h"
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nfd.h>
#include <imgui_stdlib.h>
#include "editor.h"
#include "core/types.h"
#include "core/engine.h"
#include "core/gui/imgui_helpers.h"

namespace Seed {

using namespace ImGui;
#define STATIC_GUI_FLAG                                              \
    (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | \
     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |         \
     ImGuiWindowFlags_NoCollapse)

#define PROJECT_LOADED (Editor::instance->current_project != nullptr)

void EditorGUI::main_menu() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    BeginMainMenuBar();
    main_menu_height = GetFrameHeight();
    if (BeginMenu("File")) {
        if (MenuItem("New Project") && !create_new_project) {
            project_name_input.clear();
            project_path_input.clear();
            create_new_project = true;
        }

        if (MenuItem("Load Project")) {
            nfdu8char_t *path;
            nfdopendialogu8args_t args = {0};
            nfdresult_t r = NFD_OpenDialogU8_With(&path, &args);
            if (r == NFD_OKAY) {
                gEditor->current_project = Project::load(path);
                if (gEditor->current_project) {
                    gEditor->set_last_open(gEditor->current_project->path);
                }
            }
        }
        EndMenu();
    };

    EndMainMenuBar();
    ImGui::PopStyleVar();
    if (create_new_project) {
        create_project();
    }
}
void EditorGUI::create_project() {
    SetNextWindowPos(ImVec2{200, 200});
    Begin("Create New Projct", nullptr, STATIC_GUI_FLAG);
    InputText("Project Name", &project_name_input);
    InputText("Project Path", &project_path_input);
    if (Button("Open directory")) {
        nfdu8char_t *path;
        nfdresult_t r = NFD_PickFolderU8(&path, nullptr);
        if (r == NFD_OKAY) {
            project_path_input = path;
        }
    }
    if (Button("Create")) {
        if (!project_name_input.empty() && !project_path_input.empty()) {
            Project *project = new Project();
            project->name = project_name_input;
            project->path = project_path_input;
            gEditor->current_project = project;
            project->save();
            gEditor->set_last_open(project->path);
            create_new_project = false;
        }
    }
    SameLine();
    if (Button("Cancel")) {
        create_new_project = false;
        project_error.clear();
    }
    if (!project_error.empty()) {
        Text("%s\n", project_error.data());
    }
    End();
}

void EditorGUI::main_panel() {
    Window *window = SeedEngine::get_instance()->get_window();
    ImGui::SetNextWindowPos(ImVec2(0, main_menu_height), ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(window->get_width(), window->get_height() - main_menu_height));
    ImGui::Begin("MainPanel", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

    float total_h = window->get_height() - main_menu_height;
    static float world_editor_h = 600.f;
    float asset_browser_h = total_h - world_editor_h;

    ImGui::BeginChild(
        "##top", ImVec2(0, world_editor_h), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::BeginTabBar("##TabBar")) {
        if (ImGui::BeginTabItem("Asset viewer")) {
            gEditor->asset_viewer.update();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Terrain editor")) {
            gEditor->terrain_editor.update();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    /* sizable splittter*/
    ImVec2 splitter_pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##splitter", ImVec2(-1, 4.f));
    if (ImGui::IsItemActive())
        world_editor_h =
            std::clamp(world_editor_h + ImGui::GetIO().MouseDelta.y, 100.f,
                       total_h - 100.f);
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    ImGui::GetWindowDrawList()->AddRectFilled(
        splitter_pos,
        ImVec2(splitter_pos.x + ImGui::GetContentRegionAvail().x,
               splitter_pos.y + 4.f),
        IM_COL32(60, 60, 60, 255));

    ImGui::BeginChild("##bottom", ImVec2(0, asset_browser_h - 4.f), false);
    gEditor->asset_browser.update();
    ImGui::EndChild();

    ImGui::End();
}

void EditorGUI::update() {
    static bool b = true;
    PushFont((ImFont *)font);
    main_menu();
    main_panel();
    PopFont();
}

EditorGUI::EditorGUI() {
    font = GetIO().Fonts->AddFontFromFileTTF("assets/NotoSansMono.ttf", 20);
}

}  // namespace Seed
