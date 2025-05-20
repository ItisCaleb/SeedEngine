#ifndef _SEED_RENDER_PASS_H_
#define _SEED_RENDER_PASS_H_
#include "core/rendering/api/render_command.h"
#include "core/rendering/viewport.h"

namespace Seed {
class RenderEngine;
class Renderer {
        friend RenderEngine;

    protected:
        u8 layer;

    private:
        virtual void init() = 0;
        virtual void preprocess() = 0;
        virtual void process(Viewport &viewport) = 0;
        virtual void cleanup() = 0;

    public:
        void set_layer(u32 layer) { this->layer = layer; }
};

}  // namespace Seed

#endif