#include "terrain_gui.h"
#include <fmt/format.h>
#include "core/rendering/api/render_engine.h"
#include "core/input.h"
#include "core/engine.h"
#include "core/resource/image.h"
#include "core/concurrency/thread_pool.h"

void TerrainGUI::update() {
    ImGui::Begin("Terrain");
    auto world = Seed::SeedEngine::get_instance()->get_world();
    auto terrain = world->get_terrain();

    auto &vp = Seed::RenderEngine::get_instance()->get_layer_viewport(1);
    auto input = Seed::Input::get_instance();
    auto pos = input->get_mouse_pos();
    auto dim = vp.get_actual_dimension();
    auto vp_coord = vp.to_viewport_coord(pos);
    ImGui::Text("Mouse position: %f %f", pos.x, pos.y);
    ImGui::Text("Viewport: %u %u %u %u", dim.x, dim.y, dim.w, dim.h);
    ImGui::Text("Viewport coord: %f %f", vp_coord.x, vp_coord.y);
    if (ImGui::Button("Terrain vertex")) {
        auto mat = terrain->get_material();
        auto state = mat->get_rasterizer_state();
        if (state.poly_mode == PolygonMode::FILL) {
            state.poly_mode = PolygonMode::LINE;
        } else {
            state.poly_mode = PolygonMode::FILL;
        }
        terrain->get_material()->get_texture_unit(0)->get_texture();

        mat->set_rasterizer_state(state);
    }
    if (ImGui::Button("Reset terrain")) {
        auto mat = world->get_terrain()->get_material();
        auto tex = mat->get_height_map();
        Seed::ThreadPool::get_instance()->add_work([=](void *) {
            Seed::Image image(ImageFormat::FORMAT_RGBA, 2624, 1756);
            image.fill(Color{0, 64, 0, 0}, 2624, 1756);
            image.upload(tex);
        });
    }
    ImGui::Text("Within viewport: %d", vp.within_viewport(pos.x, pos.y));
    ImGui::End();
}