#include "terrain_gui.h"
#include <fmt/format.h>
#include "core/rendering/api/render_engine.h"
#include "core/input.h"
#include "core/engine.h"
#include "core/resource/image.h"
#include "core/concurrency/thread_pool.h"
#include "core/resource/mappable_texture.h"
#include "editor.h"

using namespace Seed;

void TerrainGUI::update() {
    ImGui::Begin("Terrain");
    auto world = Seed::SeedEngine::get_instance()->get_world();
    auto terrain = Editor::get_instance()->terrain;

    auto vp = Seed::RenderEngine::get_instance()
                  ->get_render_target("default")
                  ->get_viewport();
    auto input = Seed::Input::get_instance();
    auto pos = input->get_mouse_pos();
    auto dim = vp->get_actual_dimension();
    auto vp_coord = vp->to_viewport_coord(pos);
    ImGui::Text("Mouse position: %f %f", pos.x, pos.y);
    ImGui::Text("Viewport: %f %f %f %f", dim.x, dim.y, dim.w, dim.h);
    ImGui::Text("Viewport coord: %f %f", vp_coord.x, vp_coord.y);
    if (ImGui::Button("save terrain")) {
        auto tex = terrain->get_heightmap();
        tex->save_disk("test.png");

        // Seed::ThreadPool::get_instance()->add_work([=](void *) {
        //     // Seed::Image image(PixelFormat::RGBA, 2624, 1756);
        //     // image.fill(Color{0, 64, 0, 0}, 2624, 1756);
        //     // image.upload(tex);
        // });
    }
    ImGui::Text("Within viewport: %d", vp->within_viewport(pos.x, pos.y));
    ImGui::End();
}