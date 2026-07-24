#ifndef _SEED_WORLD_RENDERER_H
#define _SEED_WORLD_RENDERER_H
#include "core/rendering/renderer/renderer.h"
#include "core/rendering/render_pass.h"
#include "core/rendering/mesh.h"
#include "core/world/sky.h"
#include <vector>
#include "core/rendering/renderer/default_renderer.h"

namespace Seed {
class WorldRenderer : public DefaultRenderer {
    private:
        Ref<MappableTexture> picking_tex;
        Ref<Texture> readback_tex;
        Ref<Texture> screen_tex;
        Ref<Texture> screen_depth;

        class ColorPass : public DefaultRenderer::ColorPass {
            public:
                void setup(Ref<Texture> main_screen, Ref<Texture> depth,
                           Ref<Texture> readback) {
                    DefaultRenderer::ColorPass::setup(main_screen, depth);
                    this->set_name("World Color Pass");
                    // older GPU doesn't support attachment with linear tiling
                    this->bind_color_attachment(readback, 0, 1);
                }
        };
        ColorPass color_pass;
    public:
        WorldRenderer(u32 screen_w, u32 screen_h);
        void reset_size(u32 screen_w, u32 screen_h);
        void init(Window *window) override;
        void _process(RenderCommandDispatcher &dp) override;
        Ref<Texture> get_screen_texture() const { return screen_tex; }
        Ref<MappableTexture> get_picking_texture() const { return picking_tex; }
};
}  // namespace Seed

#endif
