
#include <stdio.h>
#include "resource.h"
#include <nfd.h>
#include <fmt/format.h>
#include "core/engine.h"
#include "core/gui/gui_engine.h"
#include "core/resource/resource_loader.h"
#include "terrain_gui.h"
#include "editor_camera.h"

using namespace Seed;
// Main code
int main(int, char **) {
    NFD_Init();
    // Main loop
    Seed::SeedEngine *engine = new Seed::SeedEngine(60.0f);
    bool show_demo_window = true;

    // ImGui::ShowDemoWindow(&show_demo_window);
    RenderEngine::get_instance()->set_layer_viewport(
        1, {.x = 0.2, .y = 0, .w = 0.6, .h = 0.8});
    GuiEngine::get_instance()->add_gui(new ModelGUI);
    GuiEngine::get_instance()->add_gui(new TerrainGUI);

    engine->get_world()->add_entity<EditorCamera>();

    ResourceLoader *loader = ResourceLoader::get_instance();
    auto sky = loader->load_async<Sky>("assets/sky.json");
    auto terrain = loader->load_async<Terrain>("assets/iceland_heightmap.png");

    engine->get_world()->set_sky(sky->wait());
    engine->get_world()->set_terrain(terrain->wait());

    engine->start();
    NFD_Quit();
    return 0;
}
