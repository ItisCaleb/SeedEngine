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

struct RenderStateData {
        union {
                struct {
                        VertexData *instance;
                        VertexDescription *desc;
                } instance;
        };
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
        u64 gen_sort_key(RenderCommandType type, u16 depth, u16 material_id);
    public:
        void begin_state();
        void end_state();

        /* Make sure the `data` life cycle is longer than the entire frame.*/
        void update_buffer(RenderResource *buffer, u32 offset, u32 size,
                            void *data);
        void *map_buffer(RenderResource *buffer, u32 offset, u32 size);

        /* Make sure the `data` life cycle is longer than the entire frame.*/
        void update_texture(RenderResource *texture, u16 x_off, u16 y_off,
                             u16 w, u16 h, void *data);
        void *map_texture(RenderResource *buffer, u16 x_off, u16 y_off,
            u16 w, u16 h);



        RenderDispatchData generate_render_data(VertexData *vertices,
                                                VertexDescription *desc,
                                                RenderPrimitiveType prim_type,
                                                Ref<Material> mat);
        RenderDispatchData generate_render_data(VertexData *vertices,
                                                VertexDescription *desc,
                                                RenderPrimitiveType prim_type,
                                                Ref<Material> mat,
                                                RenderResource *instance,
                                                VertexDescription *instance_desc,
                                                u32 instance_cnt);
        void render(RenderDispatchData *data, RenderResource *shader);
        RenderCommandDispatcher(u8 layer): layer(layer){}

};

}  // namespace Seed

#endif