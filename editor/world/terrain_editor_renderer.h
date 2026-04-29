#ifndef _SEED_TERRAIN_EDITOR_RENDERER_H
#define _SEED_TERRAIN_EDITOR_RENDERER_H
#include "core/rendering/renderer/renderer.h"
#include "core/rendering/render_pass.h"
#include "core/rendering/mesh.h"

namespace Seed {
class TerrainEditorRenderer : public Renderer {
    private:
        struct FrameData {
                Ref<Mesh> mesh;
                u32 visible_size;
                u32 screen_w, screen_h;
                u32 mouse_x, mouse_y;
        } fd;
        Ref<MappableTexture> picking_tex;
        Ref<Texture> picking_depth;
        Ref<Texture> screen_tex;
        Ref<Texture> screen_depth;
        SSBOHandle visible_ssbo;
        SSBOHandle terrain_ssbo;
        ConstantHandle camera;

        class ColorPass : public RenderPass<FrameData> {
            public:
                void setup(Ref<Texture> main_screen, Ref<Texture> depth,
                           Ref<MappableTexture> picking_output) {
                    this->set_name("Terrain Editor Color Pass");
                    this->set_viewport(Viewport(main_screen->get_width(),
                                                main_screen->get_height()));
                    this->bind_color_attachment(main_screen, 0, 0);
                    this->bind_depth_attachment(depth, 0);
                    this->bind_color_attachment(
                        ref_cast<Texture>(picking_output), 0, 1);
                    this->set_clear_flag(CLEAR_COLOR | CLEAR_DEPTH);
                }
                void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             FrameData &fd) override;
        };

        ColorPass color_pass;

    public:
        TerrainEditorRenderer(Ref<Texture> screen_texture,
                              Ref<Texture> screen_depth, Ref<MappableTexture> picking_texture);
        void rebind_textures(Ref<Texture> screen_texture,
                              Ref<Texture> screen_depth, Ref<MappableTexture> picking_texture);
        void init(Window *window) override;
        void preprocess() override;
        void _process(RenderCommandDispatcher &dp) override;
        void cleanup() override;
};
}  // namespace Seed

#endif