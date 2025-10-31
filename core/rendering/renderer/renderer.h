#ifndef _SEED_RENDER_PASS_H_
#define _SEED_RENDER_PASS_H_
#include "core/rendering/api/render_command.h"
#include "core/rendering/viewport.h"
#include <thread>

namespace Seed {
class RenderEngine;
class Renderer {
        friend RenderEngine;

    protected:
        u8 layer;
        u8 seq = 0;
        std::mutex mu;

        u32 next_sort_key() {
            std::lock_guard<std::mutex> lock(mu);
            seq++;
            return current_sort_key();
        }

    private:
        virtual void init() = 0;
        virtual void preprocess() = 0;
        virtual void process(Viewport &viewport) = 0;
        virtual void cleanup() = 0;

    public:
        inline u32 current_sort_key(f32 depth = 0) {
            return gen_sort_key(layer, seq, depth);
        }
        void set_layer(u32 layer) { this->layer = layer; }
};

}  // namespace Seed

#endif