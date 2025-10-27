#ifndef _SEED_POST_RENDERER_H_
#define _SEED_POST_RENDERER_H_
#include "renderer.h"

namespace Seed {
class PostRenderer : public Renderer {
        friend RenderEngine;

    private:
        Ref<Material> post_mat;

    public:
        void init() override;
        void preprocess() override;
        void process(Viewport &viewport) override;
        void cleanup() override;
};

}  // namespace Seed

#endif