#include "terrain_editor.h"
#include <imgui.h>
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/resource_loader.h"

namespace Seed {
void TerrainEditor::init() {
    screen_texture.create(TextureType::TEXTURE_2D, 512, 512, PixelFormat::RGBA,
                          nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, 512, 512, PixelFormat::D32,
                        nullptr);
    ResourceLoader *loader = ResourceLoader::get_instance();
    loader->load_async<Image>(
        "assets/iceland_heightmap.png",
        [&](Ref<Image> image) { current_terrain.create(image); });
    renderer = new TerrainEditorRenderer(screen_texture, screen_depth);
    RenderEngine::get_instance()->register_renderer(1, renderer);
}
void TerrainEditor::update() {
    ImGui::Image((ImTextureID)(u64)screen_texture->get_handle(),
                 ImVec2(512, 512), ImVec2(0, 1),  // uv0
                 ImVec2(1, 0)                     // uv1
    );
}
}  // namespace Seed
