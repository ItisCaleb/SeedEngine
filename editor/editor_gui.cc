#include "editor_gui.h"
#include <spdlog/spdlog.h>
#include <nfd.h>
#include <imgui_stdlib.h>
#include "editor.h"
#include "core/types.h"
#include "core/engine.h"

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

void EditorGUI::editor_left_panel() {
    Window *window = SeedEngine::get_instance()->get_window();
    SetNextWindowSize(ImVec2(window->get_width() * 0.2, window->get_height()), 0);
    Begin("World", 0, STATIC_GUI_FLAG | ImGuiWindowFlags_NoScrollbar);
    TextUnformatted(Editor::instance->current_project->name.c_str());
    f32 remain_height = GetWindowHeight() - GetItemRectSize().y;
    Separator();
    BeginChild("World Entities", ImVec2(GetWindowWidth(), remain_height * 0.6), false);
    for (i32 i = 0; i < 20; i++) {
        Selectable(fmt::format("test{}", i).c_str());
    }
    EndChild();
    Separator();
    BeginChild("Project", ImVec2(GetWindowWidth(), remain_height * 0.3), false);
    //TreeNode();
    EndChild();
    End();
}
static bool b = true;

void EditorGUI::update() {
    PushFont((ImFont *)font);
    main_menu();
    SetNextWindowPos(ImVec2{0, main_menu_height});
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
