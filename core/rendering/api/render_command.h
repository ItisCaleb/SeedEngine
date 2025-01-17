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
    union {
        u32 offset;
        struct {
            u16 x_off;
            u16 y_off;
        } tex_off;
    };
    
    union {
        u32 size;
        struct {
            u16 w;
            u16 h;
        } tex_size;
    };
};
class RenderCommandDispatcher {
   private:
    RenderEngine *engine = nullptr;

   public:
    void begin();
    void *update(RenderResource *resource, u32 offset, u32 size);
    template <typename T>
    T *update_all(RenderResource *resource) {
        return (T *)update(resource, 0, sizeof(T));
    }

    void use(RenderResource *resource);

    void render();
    void end();
};

}  // namespace Seed

#endif