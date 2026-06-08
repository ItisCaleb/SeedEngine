#ifndef _SEED_IMGUI_RENDERER_H_
#define _SEED_IMGUI_RENDERER_H_
#include "renderer.h"
#include "core/resource/material.h"
#include "core/rendering/render_pass.h"
namespace Seed {
class ImguiRenderer : public Renderer {
    private:
        inline static ImguiRenderer *instance = nullptr;
        void create_font_material();
        struct FrameData {
                Ref<Material> font_material;
                VertexHandle vertex;
                IndexHandle indices;
        } fd;
        Window *window;
        class GUIPass : public WindowRenderPass<FrameData> {
            public:
                void setup(Window *window) {
                    this->set_name("Imgui Pass");
                    this->set_window(window);
                }
                void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             FrameData &fd) override;
        };
        GUIPass gui_pass;

        void _process(RenderCommandDispatcher &dp) override;
    public:
        void init(Window *window) override;
        void new_frame();
        void preprocess() override;
        void cleanup() override;
        static ImguiRenderer *get_instance() { return instance; }
};
}  // namespace Seed

#endif