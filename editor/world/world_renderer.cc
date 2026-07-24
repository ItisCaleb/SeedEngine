#include "world_renderer.h"
#include "editor/world/editor_world.h"
#include <cstring>
#include "editor/editor.h"
#include "core/engine.h"
#include "core/rendering/light.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/system.h"
#include "core/world/world.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {
WorldRenderer::WorldRenderer(u32 screen_w, u32 screen_h) {
    screen_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                      PixelFormat::RGBA, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                        PixelFormat::D32, nullptr);
    readback_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                        PixelFormat::RGBA16I, nullptr);
    picking_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                       PixelFormat::RGBA16I, nullptr);
}

void WorldRenderer::reset_size(u32 screen_w, u32 screen_h) {
    screen_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                      PixelFormat::RGBA, MSAAType::SAMPLE_COUNT_4,
                      SamplerProperty{}, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                        PixelFormat::D32, MSAAType::SAMPLE_COUNT_4,
                        SamplerProperty{}, nullptr);
    readback_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                        PixelFormat::RGBA16I, nullptr);
    picking_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                       PixelFormat::RGBA16I, nullptr);
    color_pass.setup(screen_tex, screen_depth, readback_tex);
}

void WorldRenderer::init(Window *window) {
    DefaultRenderer::init(window);
    color_pass.setup(screen_tex, screen_depth, readback_tex);
}

void WorldRenderer::_process(RenderCommandDispatcher &dp) {
    // dp.set_seq(0);
    // shadow_pass.draw(dp, fd);
    // dp.set_seq(1);
    color_pass.draw(dp, fd);
    readback_tex->blit_to(ref_cast<Texture>(picking_tex));
}

}  // namespace Seed
