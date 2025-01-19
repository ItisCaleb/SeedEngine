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
        struct {
            u32 offset;
            u32 size;
        } buffer;

        struct {
            u16 x_off;
            u16 y_off;
            u16 w;
            u16 h;
        } texture;
        struct {
            u32 instance_cnt;
        } render;
    };
};
class RenderCommandDispatcher {
   private:
    RenderEngine *engine = nullptr;
   public:
    void begin();
    /* The `data` parameter will be used if supplied,
       or it will allocate a new space. Use with care.*/
    /* Make sure the `data` life cycle is longer than the entire frame.*/
    void *update(RenderResource *resource, u32 offset, u32 size,
                 void *data = nullptr);
    /* The `data` parameter will be used if supplied,
   or it will allocate a new space. Use with care.*/
    /* Make sure the `data` life cycle is longer than the entire frame.*/
    void *update(RenderResource *resource, u16 x_off, u16 y_off, u16 w, u16 h,
                 void *data = nullptr);

    template <typename T>
    T *update_all(RenderResource *resource) {
        return (T *)update(resource, 0, sizeof(T));
    }

    void use(RenderResource *resource);

    void render(RenderResource *shader, u32 instance_cnt);
    void end();
    
};

}  // namespace Seed

#endif