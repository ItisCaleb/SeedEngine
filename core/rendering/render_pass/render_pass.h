#ifndef _SEED_RENDER_PASS_H_
#define _SEED_RENDER_PASS_H_
#include "core/rendering/api/render_command.h"

namespace Seed {
class RenderEngine;
class RenderPass {
    friend RenderEngine;

   private: 
    virtual void init() = 0;
    virtual void preprocess() = 0;
    virtual void process(RenderCommandDispatcher &dp, u64 sort_key) = 0;
    virtual void cleanup() = 0;
};

}  // namespace Seed

#endif