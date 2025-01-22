#ifndef _SEED_RENDERING_COMMAND_H_
#define _SEED_RENDERING_COMMAND_H_
#include "render_resource.h"
#include <queue>
#include <stack>

namespace Seed {
class RenderEngine;
enum class RenderCommandType { UPDATE, USE, RENDER };
struct RenderCommand {
    u64 sort_key;
    RenderCommandType type;
    RenderResource *resource;
    void *data;
    union {
        u32 texture_unit;
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
    bool operator<(RenderCommand const &other) {
        return sort_key < other.sort_key;
    }
};
class RenderCommandDispatcher {
   private:
    RenderEngine *engine = nullptr;
    std::stack<u64> sort_keys;
    u64 get_sort_key();

   public:
    void begin(u64 sort_key);
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

    void use(RenderResource *resource, u32 texture_unit = 0);

    void render(RenderResource *shader, u32 instance_cnt);
    void end();
};

}  // namespace Seed

#endif