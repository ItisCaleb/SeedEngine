
#include "editor.h"
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/misc/type_name.h"
#include "core/project.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/model.h"
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
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include "editor/asset/win_drag_dropper.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <nfd.h>
#include <fmt/format.h>
#include <queue>
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
    cache->write_str(project_cache.dump());
}

void Editor::set_last_open_world(const UUID uuid) {
    Ref<File> cache = File::open(".seed_cache", "wb");
    project_cache["last_open_world"] = uuid;
    cache->write_str(project_cache.dump());
}

ResourceTypeID Editor::extension_to_tid(KStr ext) {
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
        ext == ".exr" || ext == ".hdr")
        return type_id<Texture>();
    if (ext == ".world") return type_id<WorldSetting>();
    // if (ext == ".terrain")  // adjust to your format
    //     return type_id<Terrain>();
    return 0;
}

void Editor::scan_assets() {
    Project *project = System::gEngine->get_project();
    Ref<Dir> dir = Dir::open(project->get_asset_dir());
    std::queue<Ref<Dir>> dirs_to_process;
    dirs_to_process.push(dir);
    while (!dirs_to_process.empty()) {
        Ref<Dir> d = dirs_to_process.front();
        dirs_to_process.pop();
        std::vector<Path> childs = d->list();
        for (Path &path : childs) {
            path = d->concat(path);

            if (path.is_directory()) {
                dirs_to_process.push(Dir::open(path));
            } else {
                path = path.relative(project->get_path());
                if (!System::gResourceEntries->get_uuid(path).is_null())
                    continue;
                KStr extension = path.extension();
                u64 tid = extension_to_tid(extension);
                if (tid == 0) continue;
                System::gResourceEntries->insert_entry(path, tid);
            }
        }
    }
    System::gResourceEntries->save(project->get_entry_path());
}
ResourceEntry *Editor::create_asset(KStr name, ResourceTypeID tid) {
    Project *project = System::gEngine->get_project();

    Path path = "assets";
    path.push(name);
    UUID uuid = System::gResourceEntries->insert_entry(path, tid);
    System::gResourceEntries->save(project->get_entry_path());
    return System::gResourceEntries->get_entry(uuid);
}

ResourceEntry *Editor::create_internal_asset(KStr name, ResourceTypeID tid) {
    Project *project = System::gEngine->get_project();

    Path path = "assets/.internal";
    path.push(name);
    UUID uuid = System::gResourceEntries->insert_entry(path, tid);
    System::gResourceEntries->save(project->get_entry_path());
    return System::gResourceEntries->get_entry(uuid);
}

void Editor::remove_asset(UUID uuid) {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    if (!entry) return;
    File::remove(entry->real_path());
    System::gResourceEntries->remove_entry(uuid);
}

void Editor::import_asset(const Path &origin_path, const Path &target_dir) {
    Project *project = System::gEngine->get_project();

    Path moved_path = target_dir.append(origin_path.filename());
    Ref<File> origin = File::open(origin_path);
    origin->copy_to(project->get_path().append(moved_path.to_str()));

    bool r = preprocessor.try_preprocess(*System::gResourceEntries, origin,
                                         moved_path);
    if (r) {
        preprocessor.get_entries().save(project->get_preprocess_entry_path());
    } else {
        KStr extension = origin_path.extension();
        u64 tid = extension_to_tid(extension);
        if (tid == 0) return;
        System::gResourceEntries->insert_entry(moved_path, tid);
    }
    System::gResourceEntries->save(project->get_entry_path());
}

void Editor::try_open_project() {
    Ref<File> cache = File::open(".seed_cache", "rb");
    SeedEngine *engine = System::gEngine;
    if (cache.is_valid()) {
        project_cache = cache->read_json();
        if (project_cache.contains("last_open")) {
            engine->load_project(project_cache["last_open"]);
        }
    }
    Project *project = engine->get_project();
    if (engine->get_project()) {
        preprocessor.init(project->get_asset_dir());
        if (project->get_preprocess_entry_path().is_file()) {
            preprocessor.get_entries().load(
                project->get_preprocess_entry_path());
        }
        asset_browser.init(project->get_asset_dir());
        if (project_cache.contains("last_open_world")) {
            world_editor.load_world(project_cache["last_open_world"]);
        }
        scan_assets();
    }
}

void Editor::save_project() {
    Project *project = System::gEngine->get_project();
    project->save();
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
        if (System::gEngine->load_project(path)) {
            set_last_open(path);
        }
    }
}

void Editor::bind_model(Rml::Context *context) {
    if (Rml::DataModelConstructor constructor =
            context->CreateDataModel("editor")) {
        constructor.BindEventCallback("reload_shaders", [](RML_EVENT_ARGS) {
            System::gDefaultStorage->reload_shaders();
            System::gEditorStorage->reload_shaders();
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
    open_gui(&world_editor);
    open_gui(&asset_browser);
    System::gGuiEngine->load_rmlui(System::gEditor);
}

Editor::Editor() : RmlGUI(System::gEditorStorage->editor_ui_doc) {
    try_open_project();

    world_editor.init();

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

    auto &ecs = engine->get_world()->ecs();
    EditorCameraEntity::create_entity(ecs);
    ResourceLoader *loader = System::gResourceLoader;
    render_engine->set_renderer_enable(render_engine->get_default_renderer(),
                                       false);
    System::gEditor->start();
    engine->start();
    delete System::gEditorStorage;
    delete System::gEditor;
    delete engine;
    return 0;
}
// Main code
