#ifndef _SEED_RENDERING_COMMAND_H_
#define _SEED_RENDERING_COMMAND_H_
#include "render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/rendering/vertex_desc.h"
#include "core/resource/material.h"
#include "core/rendering/api/render_common.h"
#include <queue>
#include <stack>
#include <fmt/format.h>

namespace Seed {
enum class RenderCommandType : u8 { STATE, UPDATE, RENDER };
#define DEBUG_DISPATCH(_dp) \
    _dp.set_scope(fmt::format("DEBUG: {}:{}", __FILE__, __LINE__))
/*
 sort key structure
|------------------------------|
|  unused  |  layer  |  depth  |
|------------------------------|
|  9 bits  | 7 bits  | 16 bits |
|------------------------------|
*/

struct RenderCommand {
        u32 sort_key;
        RenderCommandType type;
        void *data;
        std::string scope;
        static bool cmp(RenderCommand const &a, RenderCommand const &b) {
            return a.sort_key < b.sort_key;
        }
};

struct ViewportState {
        f32 x, y, w, h;
};
struct ScissorRect {
        f32 x, y, w, h;
};

enum class DrawOperationType : u8 {
    BIND_VERTEX,
    BIND_DESC,
    BIND_INDEX,
    BIND_TEXTURE,
    UPDATE_CONSTANT,
    VIEWPORT,
    SCISSOR
};

struct DrawOperation {
        DrawOperationType type;
        union {
                struct {
                        RenderResource rc;
                } vertex;
                struct {
                        RenderResource rc;
                } index;
                struct {
                        u32 unit;
                        RenderResource rc;
                } texure;
                struct {
                        RenderResource rc;
                        void *data;
                        u32 offset;
                        u32 size;
                } constant;
                struct {
                        VertexDescription *desc;
                } desc;
                struct {
                        ViewportState rect;
                } viewport;
                struct {
                        ScissorRect rect;
                } scissor;
        };
};

struct RenderDrawData {
        u32 instance_cnt = 0;
        u32 vertex_cnt = 0;
        u32 index_offset = 0;
        u32 vertex_offset = 0;
        RenderResource pipeline;
        RenderPrimitiveType type;
        u32 operation_cnt = 0;
};

class RenderCommandDispatcher;
class RenderDrawDataBuilder {
        friend RenderCommandDispatcher;

    private:
        std::vector<u8> buffer;
        DrawOperation *alloc_operation(DrawOperationType type);
        std::vector<DrawOperation *> op_view;

    public:
        RenderDrawData *get_draw_data();

        void bind_vertex(RenderResource rc);
        void bind_index(RenderResource rc);
        void bind_vertex_data(VertexData &data, u32 offset = 0);

        void bind_texture(u32 unit, RenderResource rc);
        void bind_description(VertexDescription *desc);
        void update_constant(RenderResource rc, u32 offset, u32 size,
                             void *data);
        void set_viewport(f32 x, f32 y, f32 width, f32 height);
        void set_scissor(f32 x, f32 y, f32 width, f32 height);
        void set_draw_vertex(u32 vertex_cnt, u32 vertex_offset,
                             u32 instance_cnt = 0);
        void set_draw_index(u32 index_cnt, u32 index_offset,
                            u32 instance_cnt = 0);
        RenderDrawDataBuilder();
};

enum RenderStateFlag : u64 {
    CLEAR = 1 << 0,
    VIEWPORT = 1 << 1,
    SCISSOR = 1 << 2
};

enum StateClearFlag : u8 {
    CLEAR_COLOR = 1,
    CLEAR_DEPTH = 2,
    CLEAR_STENCIL = 4
};

struct RenderStateData {
        u64 flag;
        u8 clear_flag = 0;
        ViewportState viewport;
        ScissorRect rect;
};

struct RenderUpdateData {
        void *data;
        RenderResource rc;
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
                        u8 face;
                } texture;
        };
};

class RenderCommandDispatcher {
    private:
        u8 layer = 0;
        bool start_draw = 0;
        ViewportState viewport;
        std::string scope;
        RenderCommand prepare_state_cmd();
        RenderCommand prepare_update_cmd();

    public:
        u32 gen_sort_key(f32 depth);
        void set_scope(const std::string &scope);

        void clear(StateClearFlag flag);
        void set_viewport(f32 x, f32 y, f32 width, f32 height);
        void set_scissor(f32 x, f32 y, f32 width, f32 height);
        void cancel_scissor();

        /* Will copy data to a temporary buffer.*/
        void update_buffer(RenderResource &buffer, u32 offset, u32 size,
                           void *data);
        void *map_buffer(RenderResource &buffer, u32 offset, u32 size);

        /* Will copy data to a temporary buffer.*/
        void update_texture(RenderResource &texture, u16 x_off, u16 y_off,
                            u16 w, u16 h, void *data);
        void *map_texture(RenderResource &buffer, u16 x_off, u16 y_off, u16 w,
                          u16 h);
        void update_cubemap(RenderResource &texture, u8 face, u16 x_off,
                            u16 y_off, u16 w, u16 h, void *data);

        /* will automatically fill material state and textures */
        RenderDrawDataBuilder generate_render_data(Ref<Material> mat);

        void render(RenderDrawDataBuilder &builder, RenderPrimitiveType type,
                    RenderResource pipeline, f32 depth);

        RenderCommandDispatcher(u8 layer) : layer(layer) {}
        ~RenderCommandDispatcher();
};

}  // namespace Seed

#endif