#ifndef _SEED_RENDERING_COMMAND_H_
#define _SEED_RENDERING_COMMAND_H_
#include "render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/rendering/vertex_desc.h"
#include "core/resource/material.h"
#include <queue>
#include <stack>

namespace Seed {
class RenderEngine;
enum class RenderCommandType : u8 { STATE, UPDATE, RENDER };
enum class RenderPrimitiveType : u32 { LINES, TRIANGLES, POINTS, PATCHES };

struct RenderDispatchData {
        VertexData *vertices;
        VertexDescription *desc;
        RenderPrimitiveType prim_type;
        Ref<Material> mat;
        RenderResource *shader;
        RenderResource *instance = nullptr;
        VertexDescription *instance_desc = nullptr;
        u32 instance_cnt = 0;
};

enum RenderStateFlag : u64 {
    VIEWPORT = 1 << 0,
    CLEAR = 1 << 1,
    DEPTH = 1 << 2,
};

enum StateClearFlag : u8 { CLEAR_COLOR = 1, CLEAR_DEPTH = 2, CLEAR_STENCIL = 4 };

struct RenderStateData {
        u64 flag;
        struct Viewport {
                f32 x, y, width, height;
        } viewport;
        u8 clear_flag = 0;
};

struct RenderUpdateData {
        void *data;
        RenderResource *dst_buffer;
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
};

/*
 sort key structure
|-----------------------------------------------------|
|  unused  |  layer  |  type  |  depth  | material id |
|-----------------------------------------------------|
|  23 bits | 7 bits  | 2 bits | 16 bits |   16 bits   |
|-----------------------------------------------------|
*/

struct RenderCommand {
        u64 sort_key;
        RenderCommandType type;
        void *data;

        static bool cmp(RenderCommand const &a, RenderCommand const &b) {
            return a.sort_key < b.sort_key;
        }
};

class RenderCommandDispatcher {
    private:
        u8 layer = 0;
        RenderStateData *state_data = nullptr;
        u64 gen_sort_key(RenderCommandType type, u16 depth, u16 material_id);
        inline void ensure_state_begin();

    public:
        void begin_state();
        void set_viewport(f32 x, f32 y, f32 width, f32 height);
        void clear(StateClearFlag flag);
        void end_state();

        /* Make sure the `data` life cycle is longer than the entire frame.*/
        void update_buffer(RenderResource *buffer, u32 offset, u32 size,
                           void *data);
        void *map_buffer(RenderResource *buffer, u32 offset, u32 size);

        /* Make sure the `data` life cycle is longer than the entire frame.*/
        void update_texture(RenderResource *texture, u16 x_off, u16 y_off,
                            u16 w, u16 h, void *data);
        void *map_texture(RenderResource *buffer, u16 x_off, u16 y_off, u16 w,
                          u16 h);

        RenderDispatchData generate_render_data(VertexData *vertices,
                                                VertexDescription *desc,
                                                RenderPrimitiveType prim_type,
                                                Ref<Material> mat);
        RenderDispatchData generate_render_data(
            VertexData *vertices, VertexDescription *desc,
            RenderPrimitiveType prim_type, Ref<Material> mat,
            RenderResource *instance, VertexDescription *instance_desc,
            u32 instance_cnt);
        void render(RenderDispatchData *data, RenderResource *shader);
        RenderCommandDispatcher(u8 layer) : layer(layer) {}
        ~RenderCommandDispatcher();
};

}  // namespace Seed

#endif