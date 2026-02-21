#ifndef _SEED_RENDERER_H_
#define _SEED_RENDERER_H_
#include "core/rendering/rhi/render_command.h"
#include "core/rendering/render_pass.h"
#include "core/window.h"
#include <thread>

namespace Seed {
class Renderer {
        virtual void _process(RenderCommandDispatcher &dp) = 0;

    public:
        virtual void init(Window *window) = 0;
        virtual void preprocess() = 0;
        void process(u8 layer) {
            RenderCommandDispatcher dp;
            dp.set_layer(layer);
            _process(dp);
        }
        virtual void cleanup() = 0;
};

}  // namespace Seed

#endif