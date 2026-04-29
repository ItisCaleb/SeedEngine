
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/misc/type_name.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/model.h"
#include "core/resource/resource_entry.h"
#include <nfd.h>
#include <fmt/format.h>
#include "core/engine.h"
#include "core/gui/gui_engine.h"
#include "core/resource/resource_loader.h"
#include "camera_entity.h"
#include "editor.h"
#include "editor_storage.h"
#include "core/serialize/json_impl.h"
#include "project/project.h"

namespace Seed {
Editor *gEditor = nullptr;

Editor::Editor() {
    gEditor = this;
    Ref<File> cache = File::open(".seed_cache", "rb");
    if (cache.is_valid()) {
        project_cache = cache->read_json();
        if (project_cache.contains("last_open")) {
            this->current_project = Project::load(project_cache["last_open"]);
        }
    }
    new EditorStorage;
    GuiEngine::get_instance()->add_gui(&editor_gui);
    terrain_editor.init();
    asset_viewer.init();
    if (current_project) {
        preprocessor.init(current_project->get_asset_dir());
        if (current_project->get_preprocess_entry_path().is_file()) {
            preprocessor.get_entries().load(
                current_project->get_preprocess_entry_path());
        }
        asset_browser.init(current_project->get_asset_dir());
        if (project_cache.contains("last_open_world")) {
            terrain_editor.load_terrain(project_cache["last_open_world"]);
        }
    }
}

void Editor::set_last_open(Path &path) {
    Ref<File> cache = File::open(".seed_cache", "wb");
    project_cache["last_open"] = path;
    cache->write_str(project_cache.dump());
}

void Editor::set_last_open_world(const Path &path) {
    Ref<File> cache = File::open(".seed_cache", "wb");
    project_cache["last_open_world"] = path;
    cache->write_str(project_cache.dump());
}

void Editor::set_current_inspect(Inspectable *inspectable) {
    if (this->ctx.current_inspect != nullptr) {
        this->ctx.current_inspect->save();
        delete this->ctx.current_inspect;
        ResourceLoader::get_instance()->get_entries().save(
            current_project->get_entry_path());
    }
    this->ctx.current_inspect = inspectable;
}

void Editor::set_current_popup(Popup *popup) {
    if (this->ctx.current_popup != nullptr) {
        delete this->ctx.current_popup;
        // ResourceLoader::get_instance()->get_entries().save(
        //     current_project->get_entry_path());
    }
    this->ctx.current_popup = popup;
}

}  // namespace Seed

using namespace Seed;
int main(int, char **) {
    NFD_Init();
    // Main loop
    Seed::SeedEngine *engine = new Seed::SeedEngine(60.0f);
    RenderEngine *render_engine = RenderEngine::get_instance();
    Editor *editor = new Editor;

    auto &ecs = engine->get_world()->ecs();
    EditorCameraEntity::create_entity(ecs);
    ResourceLoader *loader = ResourceLoader::get_instance();
    render_engine->set_renderer_enable(render_engine->get_default_renderer(),
                                       false);
    

    engine->start();
    NFD_Quit();
    return 0;
}
// Main code
