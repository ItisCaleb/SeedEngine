#include "editor_gui.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <nfd.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include "core/container/kstring.h"
#include "core/engine.h"
#include "core/resource/default_storage.h"
#include "core/resource/shader.h"
#include "core/types.h"
#include "editor.h"
#include "editor_storage.h"
#include "editor/gui/editor_ui.h"
#ifdef _WIN32
#include "editor/asset/win_drag_dropper.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace Seed {

#ifdef _WIN32
static WindowDropTarget *drag_dropper = nullptr;

#endif

static constexpr ImGuiWindowFlags STATIC_GUI_FLAGS =
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse;

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
        reload_shader_if_available(ds->mesh_shader, reloaded, failed, skipped);
        reload_shader_if_available(ds->skeleton_mesh_shader, reloaded, failed,
                                   skipped);
        reload_shader_if_available(ds->sky_shader, reloaded, failed, skipped);
        reload_shader_if_available(ds->terrain_shader, reloaded, failed,
                                   skipped);
        reload_shader_if_available(ds->post_shader, reloaded, failed, skipped);
        reload_shader_if_available(ds->billboard_shader, reloaded, failed,
                                   skipped);
        reload_shader_if_available(ds->gui_shader, reloaded, failed, skipped);
        reload_shader_if_available(ds->debug_shader, reloaded, failed, skipped);
        reload_shader_if_available(ds->decal_shader, reloaded, failed, skipped);
    }

    EditorStorage *es = EditorStorage::get_instance();
    if (es != nullptr) {
        reload_shader_if_available(es->editor_terrain_shader, reloaded, failed,
                                   skipped);
    }

    shader_reload_message = "Reloaded " + std::to_string(reloaded) + " shaders";
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
    {
        EditorUI::ScopedStyleVar border(ImGuiStyleVar_WindowBorderSize, 0.0f);
        if (ImGui::BeginMainMenuBar()) {
            main_menu_height = ImGui::GetFrameHeight();
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Project") && !create_new_project) {
                    project_name_input.clear();
                    project_path_input.clear();
                    create_new_project = true;
                }

                if (ImGui::MenuItem("Load Project")) {
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
            }

            if (ImGui::BeginMenu("Tools")) {
                {
                    EditorUI::DisabledScope disabled(
                        SeedEngine::get_instance()->get_project() == nullptr);
                    if (ImGui::MenuItem("Reload All Shaders")) {
                        reload_all_shaders();
                    }
                }
                if (!shader_reload_message.empty()) {
                    ImGui::Separator();
                    ImGui::TextDisabled("%s", shader_reload_message.c_str());
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    if (create_new_project) {
        create_project();
    }
}
void EditorGUI::create_project() {
    ImGui::SetNextWindowPos(ImVec2{200, 200});
    ImGui::Begin("Create New Project", nullptr, STATIC_GUI_FLAGS);
    ImGui::InputText("Project Name", &project_name_input);
    ImGui::InputText("Project Path", &project_path_input);
    if (ImGui::Button("Open directory")) {
        nfdu8char_t *path;
        nfdresult_t r = NFD_PickFolderU8(&path, nullptr);
        if (r == NFD_OKAY) {
            project_path_input = path;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        create_new_project = false;
        project_error.clear();
    }
    if (!project_error.empty()) {
        ImGui::Text("%s\n", project_error.data());
    }
    ImGui::End();
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

    f32 total_h = (std::max)(0.0f, window->get_height() - main_menu_height);
    f32 total_w = (f32)window->get_width();
    static f32 world_editor_h = 600.f;
    static f32 inspector_w = 400.0f;
    const f32 splitter_h = 4.f;
    const f32 min_panel_h = 100.f;
    f32 max_world_editor_h = (std::max)(min_panel_h, total_h - min_panel_h);
    world_editor_h =
        std::clamp(world_editor_h, min_panel_h, max_world_editor_h);
    inspector_w =
        std::clamp(inspector_w, 260.f, (std::max)(260.f, total_w - 320.f));

    f32 asset_browser_h =
        (std::max)(0.f, total_h - world_editor_h - splitter_h);
    f32 world_editor_w = (std::max)(0.f, total_w - inspector_w);
    f32 asset_browser_w = world_editor_w;

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

    EditorUI::horizontal_splitter("##splitter", world_editor_w, splitter_h,
                                  world_editor_h, min_panel_h,
                                  max_world_editor_h);

    ImGui::BeginChild("##bottom", ImVec2(asset_browser_w, asset_browser_h),
                      false);
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
#ifdef _WIN32
    if (drag_dropper != nullptr) {
        drag_dropper->feed_gui(ImGui::GetCurrentContext());
    }
#endif
    if (font != nullptr) ImGui::PushFont((ImFont *)font);
    main_menu();
    main_panel();
    if (font != nullptr) ImGui::PopFont();
}

EditorGUI::EditorGUI() {
    font =
        ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/NotoSansMono.ttf", 20);
#ifdef _WIN32
    Window *window = SeedEngine::get_instance()->get_window();
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
    if (drag_dropper != nullptr) {
        drag_dropper->Release();
        drag_dropper = nullptr;
    }
    OleUninitialize();
#endif
}

}  // namespace Seed
