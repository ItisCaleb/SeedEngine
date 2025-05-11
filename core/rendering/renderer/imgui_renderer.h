#ifndef _SEED_IMGUI_RENDERER_H_
#define _SEED_IMGUI_RENDERER_H_
#include "renderer.h"
#include "core/resource/texture.h"
#include "core/rendering/api/render_pipeline.h"
namespace Seed {
class ImguiRenderer : public Renderer {
        friend RenderEngine;

    private:
        struct ImguiData {
                VertexData vertex;
        };
        Ref<RenderPipeline> gui_pipeline;
        Ref<Material> font_mat;
        VertexDescription desc;
        RenderResource gui_proj;
        ImguiData *get_imgui_data();
    public:
        void init() override;
        void preprocess() override;
        void process() override;
        void cleanup() override;
        ImguiRenderer(u8 layer) : Renderer(layer) {}
};
}  // namespace Seed

#endif