#ifndef _SEED_RENDER_PASS_H_
#define _SEED_RENDER_PASS_H_
#include "core/rendering/api/render_command.h"

namespace Seed {
class RenderEngine;
class Renderer {
        friend RenderEngine;
    protected:
        u8 layer;
    private:
        virtual void init() = 0;
        virtual void preprocess() = 0;
        virtual void process() = 0;
        virtual void cleanup() = 0;

    public:
        Renderer(u8 layer):layer(layer){}
};

}  // namespace Seed

#endif