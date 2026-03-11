#include "terrain_editor.h"
#include <imgui.h>
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/resource_loader.h"
#include "core/engine.h"
#include "core/input.h"

namespace Seed {
void TerrainEditor::init() {
    screen_texture.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                          PixelFormat::RGBA, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                        PixelFormat::D32, nullptr);
    picking_texture.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                           PixelFormat::RGBA16I, nullptr);
    ResourceLoader *loader = ResourceLoader::get_instance();
    loader->load_async<Image>(
        "assets/iceland_heightmap.png",
        [&](Ref<Image> image) { current_terrain.create(image); });
    renderer = new TerrainEditorRenderer(screen_texture, screen_depth,
                                         picking_texture);
    RenderEngine::get_instance()->register_renderer(1, renderer);
}
void TerrainEditor::update() {
    if (current_terrain.is_null()) {
        return;
    }
    Ref<MappableTexture> height_map = current_terrain->get_heightmap();

    Input *input = Input::get_instance();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)(u64)screen_texture->get_handle(),
                 ImVec2(screen_width, screen_height), ImVec2(0, 1),  // uv0
                 ImVec2(1, 0)                                        // uv1
    );
    Viewport vp = Viewport{screen_width, screen_height};
    auto pos = ImGui::GetMousePos();
    auto dim = vp.get_actual_dimension();
    auto vp_coord =
        vp.to_viewport_coord(Vec2{pos.x - origin.x, pos.y - origin.y});
    u32 x = vp_coord.x;
    u32 y = vp_coord.y;
    ImGui::Text("Mouse position: %f %f", pos.x, pos.y);
    ImGui::Text("Viewport: %f %f %f %f", dim.x, dim.y, dim.w, dim.h);
    ImGui::Text("Viewport coord: %f %f", vp_coord.x, vp_coord.y);
    auto terrain_coord =
        (i16 *)picking_texture->pixel_repeat(x, screen_height - y);
    i16 click_x = terrain_coord[0];
    i16 click_y = terrain_coord[1];
    ImGui::Text("Terrain coord: %d %d", click_x, click_y);
    click_x += current_terrain->get_width() / 2;
    click_y += current_terrain->get_height() / 2;
    ImGui::Text("Heightmap coord: %d %d", click_x, click_y);

    ImGui::Text("Within viewport: %d", vp.within_viewport(pos.x, pos.y));
    if (input->is_mouse_clicked(MouseEvent::LEFT)) {
        for (i32 i = -10; i < 10; i++) {
            for (i32 j = -10; j < 10; j++) {
                u32 px = click_x + i;
                u32 py = click_y + j;
                if (px >= 0 && px < height_map->get_width() && py >= 0 &&
                    py < height_map->get_height()) {
                    u8 *c = height_map->pixel(px, py);
                    if (c[1] < 255) {
                        c[1] += 1;
                    }
                }
            }
        }
    }
}
}  // namespace Seed
