#include "gui_engine.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <nfd.h>
#include <spdlog/spdlog.h>
#include "core/input.h"
#include "core/engine.h"

namespace Seed {
GuiEngine::GuiEngine(Window *window) {
    instance = this;
    if (!window) {
        SPDLOG_ERROR("Can't initialize Gui engine, window is null, exiting.");
        exit(1);
    }
    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
    NFD_Init();
}
void GuiEngine::update() {
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO &io = ImGui::GetIO();
    Input::get_instance()->set_capture_mouse(!io.WantCaptureMouse);
    for(auto gui : guis){
        gui->update();
    }
    
    ImGui::Render();
}
}  // namespace Seed