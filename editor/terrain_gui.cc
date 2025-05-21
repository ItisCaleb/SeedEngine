#include "terrain_gui.h"
#include <fmt/format.h>
#include "core/rendering/api/render_engine.h"
#include "core/input.h"

void TerrainGUI::update(){
    ImGui::Begin("Terrain");
    auto &vp = Seed::RenderEngine::get_instance()->get_layer_viewport(1);
    auto input = Seed::Input::get_instance();
    auto pos = input->get_mouse_pos();
    auto dim = vp.get_actual_dimension();
    auto vp_coord = vp.to_viewport_coord(pos);
    ImGui::Text("Mouse position: %f %f", pos.x, pos.y);
    ImGui::Text("Viewport: %u %u %u %u", dim.x, dim.y, dim.w, dim.h);
    ImGui::Text("Viewport coord: %f %f", vp_coord.x, vp_coord.y);

    ImGui::Text("Within viewport: %d", vp.within_viewport(pos.x, pos.y));
    ImGui::End();
}