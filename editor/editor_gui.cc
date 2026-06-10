#include "editor_gui.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nfd.h>
#include <imgui_stdlib.h>
#include "core/container/kstring.h"
#include "core/resource/default_storage.h"
#include "core/resource/shader.h"
#include "editor.h"
#include "editor_storage.h"
#include "core/types.h"
#include "core/engine.h"
#ifdef _WIN32
#include "editor/asset/win_drag_dropper.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace Seed {

#ifdef _WIN32
static WindowDropTarget *drag_dropper = nullptr;

#endif

using namespace ImGui;
#define STATIC_GUI_FLAG                                              \
    (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | \
     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |         \
     ImGuiWindowFlags_NoCollapse)

#define PROJECT_LOADED (SeedEngine::get_instance()->get_project() != nullptr)

static void reload_shader_if_available(Ref<Shader> shader, u32 &reloaded,
                                       u32 &failed, u32 &skipped) {
    if (shader.is_null()) {
        skipped++;
        return;
    }
    if (SeedEngine::get_instance()->get_project() == nullptr) {
        skipped++;
        return;
    }

    if (shader->reload_from_disk()) {
        reloaded++;
    } else {
        failed++;
    }
}

void EditorGUI::reload_all_shaders() {
    u32 reloaded = 0;
    u32 failed = 0;
    u32 skipped = 0;

    DefaultStorage *ds = DefaultStorage::get_instance();
    if (ds != nullptr) {
        reload_shader_if_available(ds->mesh_shader, reloaded, failed,
                                   skipped);
        reload_shader_if_available(ds->skeleton_mesh_shader, reloaded,
                                   failed, skipped);
        reload_shader_if_available(ds->sky_shader, reloaded, failed, skipped);
        reload_shader_if_available(ds->terrain_shader, reloaded, failed,
                                   skipped);
        reload_shader_if_available(ds->post_shader, reloaded, failed, skipped);
        reload_shader_if_available(ds->billboard_shader, reloaded, failed,
                                   skipped);
        reload_shader_if_available(ds->gui_shader, reloaded, failed, skipped);
        reload_shader_if_available(ds->debug_shader, reloaded, failed,
                                   skipped);
        reload_shader_if_available(ds->decal_shader, reloaded, failed,
                                   skipped);
    }

    EditorStorage *es = EditorStorage::get_instance();
    if (es != nullptr) {
        reload_shader_if_available(es->editor_terrain_shader, reloaded,
                                   failed, skipped);
    }

    shader_reload_message =
        "Reloaded " + std::to_string(reloaded) + " shaders";
    if (failed > 0) {
        shader_reload_message += ", failed " + std::to_string(failed);
    }
    if (skipped > 0) {
        shader_reload_message += ", skipped " + std::to_string(skipped);
    }
    shader_reload_message += ".";
    spdlog::info("{}", shader_reload_message);
}

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
                if (SeedEngine::get_instance()->load_project(path)) {
                    gEditor->set_last_open(path);
                }
            }
        }
        ImGui::EndMenu();
    };

    if (BeginMenu("Tools")) {
        if (!PROJECT_LOADED) ImGui::BeginDisabled();
        if (MenuItem("Reload All Shaders")) {
            reload_all_shaders();
        }
        if (!PROJECT_LOADED) ImGui::EndDisabled();
        if (!shader_reload_message.empty()) {
            ImGui::Separator();
            ImGui::TextDisabled("%s", shader_reload_message.c_str());
        }
        ImGui::EndMenu();
    }

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
    // if (Button("Create")) {
    //     if (!project_name_input.empty() && !project_path_input.empty()) {
    //         Project *project = new Project();
    //         project->name = project_name_input;
    //         project->path = project_path_input;
    //         gEditor->current_project = project;
    //         project->save();
    //         gEditor->set_last_open(project->path);
    //         create_new_project = false;
    //     }
    // }
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
    // bool show_demo_window = true;

    // ImGui::ShowDemoWindow(&show_demo_window);
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
    static float inspector_w = 400.0f;
    float asset_browser_h = total_h - world_editor_h;
    float world_editor_w = window->get_width() - inspector_w;
    float asset_browser_w = world_editor_w;

    ImGui::BeginChild(
        "##top", ImVec2(world_editor_w, world_editor_h), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::BeginTabBar("##TabBar")) {
        if (ImGui::BeginTabItem("World editor")) {
            gEditor->world_editor.update();
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
        ImVec2(splitter_pos.x + world_editor_w, splitter_pos.y + 4.f),
        IM_COL32(60, 60, 60, 255));

    ImGui::BeginChild("##bottom",
                      ImVec2(asset_browser_w, asset_browser_h - 4.f), false);
    gEditor->asset_browser.update();
    ImGui::EndChild();

    ImGui::SetCursorScreenPos(ImVec2(world_editor_w, main_menu_height));
    ImGui::BeginChild("##inspector", ImVec2(inspector_w, total_h), false);
    gEditor->inspector.update();
    ImGui::EndChild();

    if (gEditor->ctx.current_popup != nullptr) {
        gEditor->ctx.current_popup->draw();
        if (gEditor->ctx.current_popup->should_close) {
            gEditor->set_current_popup(nullptr);
        }
    }

    ImGui::End();
}

void EditorGUI::update() {
    static bool b = true;
    drag_dropper->feed_gui(ImGui::GetCurrentContext());
    PushFont((ImFont *)font);
    main_menu();
    main_panel();
    PopFont();
}

EditorGUI::EditorGUI() {
    font = GetIO().Fonts->AddFontFromFileTTF("assets/NotoSansMono.ttf", 20);
    Window *window = SeedEngine::get_instance()->get_window();
#ifdef _WIN32

    // GLFW 取得 HWND
    HWND hwnd = glfwGetWin32Window(window->get_window<GLFWwindow>());
    OleInitialize(nullptr);
    drag_dropper = new WindowDropTarget();
    RegisterDragDrop(hwnd, drag_dropper);
#endif
}

EditorGUI::~EditorGUI() {
#ifdef _WIN32
    Window *window = SeedEngine::get_instance()->get_window();

    HWND hwnd = glfwGetWin32Window(window->get_window<GLFWwindow>());

    RevokeDragDrop(hwnd);
    drag_dropper->Release();
    OleUninitialize();
#endif
}

}  // namespace Seed
