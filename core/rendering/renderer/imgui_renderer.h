#ifndef _SEED_IMGUI_RENDERER_H_
#define _SEED_IMGUI_RENDERER_H_
#include "renderer.h"
#include "core/resource/texture.h"
#include "core/resource/material.h"
namespace Seed {
class ImguiRenderer : public Renderer {
        friend RenderEngine;

    private:
        inline static ImguiRenderer *instance = nullptr;
        struct ImguiData {
                VertexHandle vertex;
                IndexHandle indices;
        };
        Ref<Material> font_mat;
        ImguiData *get_imgui_data();
        Ref<Texture> create_font_texture();

    public:
        void init() override;
        void new_frame();
        void preprocess() override;
        void process(Viewport &viewport) override;
        void cleanup() override;
        static ImguiRenderer *get_instance() { return instance; }
};
}  // namespace Seed

#endif