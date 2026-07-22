#include "gui_engine.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Input.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <nfd.h>
#include <spdlog/spdlog.h>
#include "core/input.h"
#include "core/rendering/renderer/imgui_renderer.h"
#include "core/gui/rml_widgets.h"

namespace Seed {

void GuiEngine::add_texture(KStr name, Ref<Texture> texture) {
    stored_texture[name] = texture;
}

Ref<Texture> GuiEngine::get_texture(KStr name) {
    auto iter = stored_texture.find(name);
    if (iter == stored_texture.end()) {
        return Ref<Texture>();
    }
    return iter->second;
}

void GuiEngine::register_seed_rml(Rml::Context *context) {
    Rml::DataModelConstructor constructor = context->CreateDataModel("global");
    constructor.RegisterScalar<KString>(
        [](const KString &str, Rml::Variant &variant) {
            variant = Rml::String(str.data(), str.size());
        },
        [](KString &str, const Rml::Variant &variant) {
            str = variant.Get<Rml::String>();
        });

    constructor.RegisterScalar<UUID>(
        [](const UUID &uuid, Rml::Variant &variant) {
            variant = uuid.to_string();
        },
        [](UUID &uuid, const Rml::Variant &variant) {
            uuid = UUID::from_string(variant.Get<Rml::String>());
        });
    Rml::RegisterEventType("hover", true, true);
}

GuiEngine::GuiEngine(Window *window) {
    this->window = window;
    if (!window) {
        SPDLOG_ERROR("Can't initialize Gui engine, window is null, exiting.");
        exit(1);
    }
    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(glfw_window, true);
    NFD_Init();

    Rml::SetSystemInterface(&rml_system_interface);
    Rml::SetRenderInterface(&rml_interface);
    Rml::Initialise();
    Rml::LoadFontFace("assets/NotoSansMono.ttf", true);
    rml_context = Rml::CreateContext(
        "main",
        Rml::Vector2i((int)window->get_width(), (int)window->get_height()));
    rml_element_instancer.RegisterElements();
    register_seed_rml(rml_context);
}

GuiEngine::~GuiEngine() {
    Rml::RemoveContext("main");
    Rml::Shutdown();
    NFD_Quit();
    ImGui_ImplGlfw_Shutdown();
    if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
}

void GuiEngine::update_rml_context() {
    if (!rml_context) return;
    rml_context->SetDimensions(
        Rml::Vector2i((int)window->get_width(), (int)window->get_height()));
}

void GuiEngine::update() {
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    update_rml_context();

    ImGuiIO &io = ImGui::GetIO();
    for (auto gui : guis) {
        gui->update();
    }

    if (rml_context) {
        rml_context->Update();
        Rml::Element *hover = rml_context->GetHoverElement();
        if (hover != nullptr) {
            Rml::Dictionary params;
            Vec2i pos = System::gInput->get_mouse_actual_pos();

            params["mouse_x"] = pos.x;
            params["mouse_y"] = pos.y;
            hover->DispatchEvent("hover", params);
        }
        rml_interface.begin_frame();
        rml_context->Render();
    }

    ImGui::Render();
}
}  // namespace Seed