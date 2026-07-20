#ifndef _SEED_WORLD_RENDERER_H
#define _SEED_WORLD_RENDERER_H
#include "core/rendering/renderer/renderer.h"
#include "core/rendering/render_pass.h"
#include "core/rendering/mesh.h"
#include <vector>

namespace Seed {
class WorldRenderer : public Renderer {
    private:
        struct FrameData {
                struct StaticMesh {
                        Ref<Mesh> mesh;
                        u32 visible_offset = 0;
                        u32 visible_size = 0;
                };

                Ref<Mesh> mesh;
                u32 visible_size = 0;
                u32 screen_w, screen_h;
                u32 mouse_x, mouse_y;
                std::vector<StaticMesh> static_meshes;
        } fd;
        Ref<MappableTexture> picking_tex;
        Ref<Texture> readback_tex;
        Ref<Texture> screen_tex;
        Ref<Texture> screen_depth;

        class ColorPass : public RenderPass<FrameData> {
            public:
                void setup(Ref<Texture> main_screen, Ref<Texture> depth,
                           Ref<Texture> readback) {
                    this->set_name("Terrain Editor Color Pass");
                    this->set_viewport(Viewport(main_screen->get_width(),
                                                main_screen->get_height()));
                    this->bind_color_attachment(main_screen, 0, 0);
                    this->bind_depth_attachment(depth, 0);
                    // older GPU doesn't support attachment with linear tiling
                    this->bind_color_attachment(readback, 0, 1);
                    this->set_clear_flag(CLEAR_COLOR | CLEAR_DEPTH);
                }
                void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             FrameData &fd) override;
        };

        ColorPass color_pass;

    public:
        WorldRenderer(u32 screen_w, u32 screen_h);
        void reset_size(u32 screen_w, u32 screen_h);
        void init(Window *window) override;
        void preprocess() override;
        void _process(RenderCommandDispatcher &dp) override;
        void cleanup() override;
        Ref<Texture> get_screen_texture() const { return screen_tex; }
        Ref<MappableTexture> get_picking_texture() const { return picking_tex; }
};
}  // namespace Seed

#endif
