#ifndef _SEED_RENDERING_COMMAND_H_
#define _SEED_RENDERING_COMMAND_H_
#include "render_resource.h"
#include <queue>

namespace Seed {
class RenderEngine;
enum class RenderCommandType { UPDATE, USE, RENDER };
struct RenderCommand {
    RenderCommandType type;
    RenderResource *resource;
    void *data;
    u32 offset;
    u32 size;
};
class RenderCommandDispatcher {
   private:
    RenderEngine *engine = nullptr;

   public:
    void begin();
    void* update(RenderResource *resource, u32 offset, u32 size);
    void use(RenderResource *resource);

    void render();
    void end();
};

}  // namespace Seed

#endif