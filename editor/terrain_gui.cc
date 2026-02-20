#include "terrain_gui.h"
#include <fmt/format.h>
#include "core/rendering/rhi/render_engine.h"
#include "core/input.h"
#include "core/engine.h"
#include "core/resource/image.h"
#include "core/concurrency/thread_pool.h"
#include "core/resource/mappable_texture.h"
#include "editor.h"
#include "core/resource/resource_loader.h"

using namespace Seed;

TerrainGUI::TerrainGUI() {
    ResourceLoader *loader = ResourceLoader::get_instance();
    loader->load_async<Image>("assets/iceland_heightmap.png",
                              [&](Ref<Image> image) { terrain.create(image); });
    auto rt = ref_cast<MultiRenderTarget>(
        RenderEngine::get_instance()->get_render_target("default"));
    auto rect = rt->get_viewport()->get_actual_dimension();
    terrain_pos_tex.create(TextureType::TEXTURE_2D, rect.w, rect.h,
                           PixelFormat::RGBA16I, nullptr);
    rt->bind_color(1, ref_cast<Texture>(terrain_pos_tex));
}

void TerrainGUI::update() {
    ImGui::Begin("Terrain");
    auto world = Seed::SeedEngine::get_instance()->get_world();

    auto vp = Seed::RenderEngine::get_instance()
                  ->get_render_target("default")
                  ->get_viewport();
    auto input = Seed::Input::get_instance();
    auto pos = input->get_mouse_pos();
    auto dim = vp->get_actual_dimension();
    auto vp_coord = vp->to_viewport_coord(pos);
    u32 x = vp_coord.x * dim.w;
    u32 y = vp_coord.y * dim.h;
    ImGui::Text("Mouse position: %f %f", pos.x, pos.y);
    ImGui::Text("Viewport: %f %f %f %f", dim.x, dim.y, dim.w, dim.h);
    ImGui::Text("Viewport coord: %f %f", vp_coord.x, vp_coord.y);
    ImGui::Text("Viewport actual coord: %u %u", x, y);
    auto terrain_coord = (i16 *)terrain_pos_tex->pixel_repeat(x, y);
    ImGui::Text("Terrain coord: %d %d", terrain_coord[0], terrain_coord[1]);

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