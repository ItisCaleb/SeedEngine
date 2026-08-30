
#include "editor.h"

#include <utility>

#include "core/io/path.h"
#include "core/misc/type_name.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/resource_entry.h"
#include "core/resource/world_setting.h"
#include "core/engine.h"
#include "core/gui/gui_engine.h"
#include "core/resource/resource_loader.h"
#include "core/serialize/json_impl.h"
#include "core/concurrency/thread_pool.h"
#include "core/window.h"
#include "core/world/world.h"
#include "editor/camera_entity.h"
#include "editor/editor_storage.h"
#include "editor/gui/editor_widget.h"
#include "editor/project/project.h"
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include "editor/asset/win_drag_dropper.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <nfd.h>
#include <RmlUi/Core/Factory.h>

namespace Seed {

namespace System {
Editor *gEditor = nullptr;
EditorStorage *gEditorStorage = nullptr;
}  // namespace System
#ifdef _WIN32
static WindowDropTarget *drag_dropper = nullptr;

#endif

void Editor::set_last_open(const Path &path) {
    Ref<File> cache = File::open(".seed_cache", "wb");
    project_cache["last_open"] = path;
    project_cache.erase("last_open_world");
    cache->write_str(project_cache.dump());
}

void Editor::set_last_open_world(const UUID uuid) {
    Ref<File> cache = File::open(".seed_cache", "wb");
    project_cache["last_open_world"] = uuid;
    cache->write_str(project_cache.dump());
}

void Editor::close_world() {
    world_entry = nullptr;
    world_setting = {};
    world = nullptr;
    world_viewport.reset();
}

bool Editor::load_world(ResourceEntry *entry) {
    close_world();
    if (entry == nullptr || System::gEngine == nullptr ||
        System::gResourceLoader == nullptr) {
        return false;
    }

    World *loaded_world = System::gEngine->get_world();
    if (loaded_world == nullptr) return false;

    Ref<WorldSetting> loaded_setting =
        System::gResourceLoader->load<WorldSetting>(entry->uuid);
    if (loaded_setting.is_null()) loaded_setting.create();

    loaded_world->load_setting(loaded_setting);
    world_entry = entry;
    world_setting = loaded_setting;
    world = loaded_world;
    return true;
}

bool Editor::save_world() {
    if (!has_world() || project.is_null()) return false;

    nlohmann::ordered_json &json = world_entry->config.get_json();
    if (!json.is_object()) json = nlohmann::ordered_json::object();

    json["name"] = world_setting->name;
    json["sky"] = {{"up", world_setting->sky.up},
                   {"down", world_setting->sky.down},
                   {"left", world_setting->sky.left},
                   {"right", world_setting->sky.right},
                   {"front", world_setting->sky.front},
                   {"back", world_setting->sky.back}};
    json["directional_light"] = {
        {"direction", world_setting->dir_light.direction},
        {"diffuse", world_setting->dir_light.diffuse},
        {"specular", world_setting->dir_light.specular}};
    json["terrain_textures"] = world_setting->terrain_textures;
    json["terrain_normals"] = world_setting->terrain_normals;
    json["chunks"] = nlohmann::ordered_json::array();

    for (const ChunkSetting &chunk : world_setting->chunks) {
        nlohmann::ordered_json chunk_json = {
            {"x", chunk.x},
            {"y", chunk.y},
            {"height_map", chunk.height_map},
            {"control_map", chunk.control_map},
            {"position_lights", nlohmann::ordered_json::array()},
            {"static_objects", nlohmann::ordered_json::array()}};

        for (const PointLightSetting &light : chunk.lights) {
            chunk_json["position_lights"].push_back(
                {{"position", light.position},
                 {"diffuse", light.diffuse},
                 {"specular", light.specular}});
        }
        for (const StaticObjectSetting &object : chunk.static_objects) {
            chunk_json["static_objects"].push_back({{"name", object.name},
                                                    {"x", object.x},
                                                    {"y", object.y},
                                                    {"z", object.z},
                                                    {"model", object.model}});
        }

        json["chunks"].push_back(std::move(chunk_json));
    }

    project->save();
    return true;
}

bool Editor::has_world() const {
    return world_entry != nullptr && world_setting.is_valid() &&
           world != nullptr;
}

bool Editor::open_project(const Path &path) {
    Ref<Project> loaded_project = Project::load(path);
    if (loaded_project.is_null()) return false;

    project = loaded_project;
    project->scan_assets();
    close_world();
    world_editor_panel.on_project_changed();
    asset_browser.init(project);
    return true;
}

void Editor::try_open_project() {
    Ref<File> cache = File::open(".seed_cache", "rb");
    if (cache.is_null()) return;

    project_cache = cache->read_json();
    if (!project_cache.contains("last_open") ||
        !open_project(project_cache["last_open"])) {
        return;
    }
    if (project_cache.contains("last_open_world")) {
        open_asset(project_cache["last_open_world"]);
    }
}

void Editor::new_project(RML_EVENT_ARGS) {
    if (!show_create) {
        project_name_input.clear();
        project_path_input.clear();
        show_create = true;
        if (project_model) project_model.DirtyVariable("show_create");
    }
}

void Editor::load_project(RML_EVENT_ARGS) {
    nfdu8char_t *path;
    nfdopendialogu8args_t _args = {0};
    nfdresult_t r = NFD_OpenDialogU8_With(&path, &_args);
    if (r == NFD_OKAY) {
        if (open_project(path)) {
            set_last_open(path);
        }
        NFD_FreePathU8(path);
    }
}

bool Editor::open_asset(UUID uuid) {
    if (project.is_null()) return false;

    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    if (entry == nullptr) return false;
    if (entry->type_id != type_id<WorldSetting>()) return false;

    const bool loaded = load_world(entry);
    world_editor_panel.on_world_changed(loaded);
    if (!loaded) return false;
    set_last_open_world(uuid);
    return true;
}

void Editor::bind_model(Rml::Context *context) {
    if (Rml::DataModelConstructor constructor =
            context->CreateDataModel("editor")) {
        constructor.BindEventCallback("reload_shaders", [](RML_EVENT_ARGS) {
            System::gDefaultStorage->reload_shaders();
            System::gEditorStorage->reload_shaders();
            spdlog::info("Reloaded shaders");
        });
        constructor.BindEventCallback("reload_gui",
                                      [=](RML_EVENT_ARGS) { this->reload(); });
        constructor.BindEventCallback("new_project", &Editor::new_project,
                                      this);
        constructor.BindEventCallback("load_project", &Editor::load_project,
                                      this);
    }
    if (Rml::DataModelConstructor constructor =
            context->CreateDataModel("project_create")) {
        constructor.Bind("show_create", &show_create);
        constructor.Bind("name", &project_name_input);
        constructor.Bind("path", &project_path_input);
        project_model = constructor.GetModelHandle();
    }
}

void Editor::start() {
    open_gui(&world_editor_panel);
    open_gui(&inspector_panel);
    open_gui(&asset_browser);
    System::gGuiEngine->load_rmlui(System::gEditor);

    EditorCameraEntity::create_entity(System::gEngine->get_world()->ecs(),
                                      world_viewport);
}

Editor::Editor()
    : RmlGUI(System::gEditorStorage->editor_ui_doc),
      world_editor_panel(inspector_panel) {
    System::gEditor = this;
    world_viewport.init();
    world_editor_panel.init();
    try_open_project();

#ifdef _WIN32
    Window *window = System::gEngine->get_window();
    HWND hwnd = glfwGetWin32Window(window->get_window<GLFWwindow>());
    OleInitialize(nullptr);
    drag_dropper = new WindowDropTarget();
    RegisterDragDrop(hwnd, drag_dropper);
#endif
}

Editor::~Editor() {
#ifdef _WIN32
    Window *window = System::gEngine->get_window();

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

using namespace Seed;
i32 main(i32, char **) {
    // Main loop
    Seed::SeedEngine *engine = new Seed::SeedEngine(60.0f);
    RenderEngine *render_engine = System::gRenderEngine;
    EditorRmlElementInstancer rml_instancer;
    rml_instancer.RegisterElements();

    System::gEditorStorage = new EditorStorage;

    System::gEditor = new Editor;

    render_engine->set_renderer_enable(render_engine->get_default_renderer(),
                                       false);
    System::gEditor->start();
    engine->start();
    delete System::gEditor;
    System::gEditor = nullptr;
    delete System::gEditorStorage;
    System::gEditorStorage = nullptr;
    delete engine;
    return 0;
}
// Main code
