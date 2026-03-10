
#include <stdio.h>
#include "editor/asset/model_loader.h"
#include <nfd.h>
#include <fmt/format.h>
#include "core/engine.h"
#include "core/gui/gui_engine.h"
#include "editor_gui.h"
#include "core/resource/resource_loader.h"
#include "editor_camera.h"
#include "editor.h"
#include "editor_storage.h"

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
    GuiEngine::get_instance()->add_gui(new EditorGUI);
    terrain_editor.init();
    asset_viewer.init();
}

void Editor::set_last_open(std::string &path) {
    Ref<File> cache = File::open(".seed_cache", "wb");
    project_cache["last_open"] = path;
    cache->write_str(project_cache.dump());
}
}  // namespace Seed

// Main code
int main(int, char **) {
    NFD_Init();
    // Main loop
    Seed::SeedEngine *engine = new Seed::SeedEngine(60.0f);
    RenderEngine *render_engine = RenderEngine::get_instance();
    Editor *editor = new Editor;

    bool show_demo_window = true;

    // ImGui::ShowDemoWindow(&show_demo_window);
    // GuiEngine::get_instance()->add_gui(new ModelGUI);
    // GuiEngine::get_instance()->add_gui(new TerrainGUI);

    engine->get_world()->add_entity<EditorCamera>();
    ResourceLoader *loader = ResourceLoader::get_instance();
    render_engine->set_renderer_enable(render_engine->get_default_renderer(),
                                       false);
    // auto sky = loader->load_async<Sky>("assets/sky.json");
    // engine->get_world()->set_sky(sky->wait());
    // engine->get_world()->set_terrain(terrain->wait());

    engine->start();
    NFD_Quit();
    return 0;
}
