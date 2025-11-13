#include "editor_gui.h"
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

bool create_new_project;
std::string project_name_input;
std::string project_path_input;
std::string project_error;
float main_menu_height;

void EditorGUI::main_menu() {
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
                Editor::instance->current_project = Project::load(path);
                if (Editor::instance->current_project) {
                    Editor::instance->set_last_open(
                        Editor::instance->current_project->path);
                }
            }
        }
        EndMenu();
    };

    EndMainMenuBar();
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
            Editor::instance->current_project = project;
            project->save();
            Editor::instance->set_last_open(project->path);
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

void EditorGUI::character_database() {}

void EditorGUI::editor_left_panel() {
    Window *window = SeedEngine::get_instance()->get_window();
    SetNextWindowPos(ImVec2(0, main_menu_height), ImGuiCond_Always);

    ImGui::Begin("LeftPanel", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);

    static int currentTab = -1;
    const VerticalIconTab kTabs[] = {
        {"S", "Settings", [] {}},
        {"T", "NPCs", [&] { this->character_database(); }},
        {"S", "Scripts", [] { ImGui::Text("Script editor"); }},
        {"P", "Profile", [] { ImGui::Text("User profile"); }},
    };

    drawLVerticalIconTabs(kTabs, IM_ARRAYSIZE(kTabs), currentTab);
    End();
}

void EditorGUI::update() {
    static bool b = true;
    PushFont((ImFont *)font);
    main_menu();
    if (PROJECT_LOADED) {
        editor_left_panel();
    }
    ShowDemoWindow(&b);
    PopFont();
}

EditorGUI::EditorGUI() {
    font = GetIO().Fonts->AddFontFromFileTTF("assets/NotoSansMono.ttf", 20);
}

}  // namespace Seed
