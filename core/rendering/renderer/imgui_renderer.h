#ifndef _SEED_IMGUI_RENDERER_H_
#define _SEED_IMGUI_RENDERER_H_
#include "renderer.h"
#include "core/resource/texture.h"
#include "core/resource/material.h"
namespace Seed {
class ImguiRenderer : public Renderer {
        friend RenderEngine;

    private:
        struct ImguiData {
                VertexData vertex;
        };
        Ref<Material> font_mat;
        RenderResource gui_proj;
        ImguiData *get_imgui_data();

    public:
        void init() override;
        void preprocess() override;
        void process(Viewport &viewport) override;
        void cleanup() override;
};
}  // namespace Seed

#endif