#ifndef _SEED_RML_RENDERER_H_
#define _SEED_RML_RENDERER_H_

#include "renderer.h"
#include "core/resource/material.h"
#include "core/rendering/render_pass.h"

namespace Seed {

class RmlRenderer : public Renderer {
    private:
        struct FrameData {
                Ref<Material> material;
        } fd;
        Window *window;

        class GUIPass : public WindowRenderPass<FrameData> {
            public:
                void setup(Window *window) {
                    this->set_name("RmlUi Pass");
                    this->set_window(window);
                }
                void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             FrameData &fd) override;
        } gui_pass;

        void create_material();
        void _process(RenderCommandDispatcher &dp) override;

    public:
        void init(Window *window) override;
        void preprocess() override;
        void cleanup() override;
};

}  // namespace Seed

#endif