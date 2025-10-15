#include "editor_gui.h"
#include <spdlog/spdlog.h>
#include <nfd.h>
#include <imgui_stdlib.h>
#include "editor.h"

namespace Seed {

using namespace ImGui;

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
        EndMenu();
    };

    EndMainMenuBar();
    if (create_new_project) {
        create_project();
    }
}
void EditorGUI::create_project() {
    SetNextWindowPos(ImVec2{200, 200});
    Begin("Create New Projct", nullptr,
          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
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

void EditorGUI::update() {
    main_menu();
    SetNextWindowPos(ImVec2{0, main_menu_height});
    Begin("Editor", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
    End();
}
}  // namespace Seed
