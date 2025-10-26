#ifndef _SEED_RENDERING_COMMAND_H_
#define _SEED_RENDERING_COMMAND_H_
#include "render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/rendering/vertex_layout.h"
#include "core/resource/material.h"
#include "core/rendering/render_common.h"
#include "core/collision/shape.h"
#include "core/rendering/viewport.h"
#include <queue>
#include <stack>
#include <fmt/format.h>

namespace Seed {
enum class RenderCommandType : u8 {
    STATE,
    UPDATE,
    RENDER,
    BEGIN_SCOPE,
    END_SCOPE
};

#define KEY_DEPTH_BITS (16)
#define KEY_DEPTH_MASK ((1 << KEY_DEPTH_BITS) - 1)
#define KEY_SEQ_BITS (8)
#define KEY_SEQ_MASK ((1 << 8) - 1)
#define KEY_LAYER_BITS (6)
#define KEY_LAYER_MASK ((1 << KEY_LAYER_BITS) - 1)

/*
 sort key structure

|  unused  |  layer  |  sequence  |  depth  |
|----------|---------|------------|---------|
|  2 bits  | 6 bits  |   8 bits   | 16 bits |

*/
static u32 gen_sort_key(u8 layer, u8 sequence, f32 depth) {
    u64 sort_key = 0;
    if (depth < 0.0f) depth = 0.0f;
    if (depth > 1.0f) depth = 1.0f;
    u16 depth_value = (u16)((f32)(u16)(-1) * depth);
    sort_key |= ((u32)layer & KEY_LAYER_MASK)
                << (KEY_DEPTH_BITS + KEY_SEQ_BITS);
    sort_key |= ((u32)sequence & KEY_SEQ_MASK) << (KEY_DEPTH_BITS);
    sort_key |= (u32)depth_value;
    return sort_key;
}

struct RenderCommand {
        u32 sort_key;
        RenderCommandType type;
        void *data;
        static bool cmp(RenderCommand const &a, RenderCommand const &b) {
            return a.sort_key < b.sort_key;
        }
};

struct RenderDrawData {
        enum class OpType : u8 {
            BIND_VERTEX,
            BIND_DESC,
            BIND_INDEX,
            BIND_TEXTURE,
            VIEWPORT,
            SCISSOR
        };
        struct Operation {
                OpType type;
                union {
                        RenderResource vertex_rc;
                        RenderResource index_rc;
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
                        VertexLayout *vertex_desc;
                        RectF view_rect;
                        RectF scissor_rect;
                };
        };

        u32 instance_cnt = 0;
        u32 instance_offset = 0;
        u32 vertex_cnt = 0;
        u32 index_offset = 0;
        u32 vertex_offset = 0;
        RenderResource pipeline;
        RenderPrimitiveType type;
        u32 operation_cnt = 0;
};

/*
 RenderDrawDataBuilder is use to build RenderDrawCommmand
|------------------------------|-------|-------|
|  RenderDrawCommmand  |  op1  |  op2  | ..... |
|------------------------------|-------|-------|
*/
template <typename T>
class DataBuilder {
    protected:
        std::vector<u8> buffer;
        std::vector<T *> op_view;
        typename T::Operation *alloc_operation(typename T::OpType type) {
            this->buffer.resize(this->buffer.size() +
                                sizeof(typename T::Operation));
            typename T::Operation *dst =
                (typename T::Operation *)(this->buffer.data() +
                                          this->buffer.size() -
                                          sizeof(typename T::Operation));
            dst->type = type;
            T *data = static_cast<T *>((void *)&this->buffer[0]);
            data->operation_cnt++;

            /* for debug */
            op_view.resize(data->operation_cnt);
            for (i32 i = 0; i < data->operation_cnt; i++) {
                op_view[i] = (T *)((u64)data + sizeof(T) +
                                   i * sizeof(typename T::Operation));
            }
            return dst;
        }

    public:
        T *get_data() { return static_cast<T *>((void *)&this->buffer[0]); }
        DataBuilder() { this->buffer.resize(sizeof(T)); }
};

class RenderCommandDispatcher;
class RenderDrawDataBuilder : public DataBuilder<RenderDrawData> {
        friend RenderCommandDispatcher;

    public:
        void bind_vertex(RenderResource rc);
        void bind_index(RenderResource rc);
        void bind_vertex_data(Ref<VertexData> data, u32 offset = 0);
        void bind_index_data(Ref<IndexData> data, u32 offset = 0);

        void bind_texture(u32 unit, RenderResource rc);
        void bind_description(VertexLayout *desc);
        void set_viewport(f32 x, f32 y, f32 width, f32 height);
        void set_scissor(f32 x, f32 y, f32 width, f32 height);
        void set_draw_vertex(u32 vertex_cnt, u32 vertex_offset);
        void set_draw_index(u32 index_cnt, u32 index_offset);
        void set_instance(u32 cnt, u32 instance_offset = 0);
};

enum StateClearFlag : u8 {
    CLEAR_COLOR = 1,
    CLEAR_DEPTH = 2,
    CLEAR_STENCIL = 4
};

struct RenderStateData {
        enum class OpType : u8 {
            BIND_RENDER_TARGET,
            VIEWPORT,
            SCISSOR,
            CLEAR,
            BIND_BUFFERBASE
        };
        struct Operation {
                OpType type;
                union {
                        RenderResource render_target;
                        struct {
                                RectF *view_rects;
                                u32 counts;
                        } viewports;
                        RectF scissor_rect;
                        u8 clear_flag;
                        struct {
                                RenderResource buffer;
                                u32 base;
                        } bufferbase;
                };
        };
        u32 operation_cnt = 0;
};

class RenderStateDataBuilder : public DataBuilder<RenderStateData> {
        friend RenderCommandDispatcher;

    public:
        void bind_render_target(RenderResource target);
        void bind_window();
        void set_viewport(Viewport *viewport);
        void set_viewports(std::vector<Viewport> &viewports);

        void set_scissor(f32 x, f32 y, f32 width, f32 height);
        void set_scissor(const RectF &scissor_rect);
        void bind_bufferbase(RenderResource buffer, u32 base);

        void clear(StateClearFlag flag);
};

struct RenderUpdateData {
        RenderResource rc;
        bool filled;
        union {
                struct {
                        u32 offset;
                        u32 size;
                } buffer;

                struct {
                        u32 x_off;
                        u32 y_off;
                        u32 w;
                        u32 h;
                        u8 face;
                } texture;

                /* slot -1 for depth attachment */
                struct {
                        u32 face;
                        RenderResource texture;
                        i32 slot;
                } attachment{};
        };
        void *get_buffer() {
            return (void *)((u64)this + sizeof(RenderUpdateData));
        }

        void set_filled() { this->filled = true; }
};

class RenderCommandQueue;
class RenderCommandDispatcher {
        friend RenderDrawDataBuilder;

    private:
        RectF viewport;
        void *push_update_cmd(RenderUpdateData &update_data, u32 sort_key,
                              u64 size, void *data);

    public:
        void begin_scope(const std::string &scope, u32 sort_key);
        void end_scope(u32 sort_key);

        void set_states(RenderStateDataBuilder &builder, u32 sort_key);
        /* Will copy data to a temporary buffer.*/
        void update_buffer(const RenderResource &buffer, u32 offset, u32 size,
                           void *data, u32 sort_key = 0);
        RenderUpdateData *map_buffer(const RenderResource &buffer, u32 offset,
                                     u32 size, u32 sort_key = 0);

        /* Will copy data to a temporary buffer.*/
        void update_texture(const RenderResource &texture, u32 x_off, u32 y_off,
                            u32 w, u32 h, void *data, u32 sort_key = 0);
        RenderUpdateData *map_texture(const RenderResource &buffer, u32 x_off,
                                      u32 y_off, u32 w, u32 h,
                                      u32 sort_key = 0);
        void update_cubemap(const RenderResource &texture, u8 face, u16 x_off,
                            u16 y_off, u16 w, u16 h, void *data,
                            u32 sort_key = 0);
        void update_color_attachment(const RenderResource &render_target,
                                     i32 slot, RenderResource tex, u32 face = 0,
                                     u32 sort_key = 0);
        void update_depth_attachment(const RenderResource &render_target,
                                     RenderResource tex, u32 face = 0,
                                     u32 sort_key = 0);

        /* will automatically fill material state and textures */
        RenderDrawDataBuilder generate_render_data(Ref<Material> mat);

        void render(RenderDrawDataBuilder &builder, RenderPrimitiveType type,
                    RenderResource pipeline, u32 sort_key);

        RenderCommandDispatcher();
        ~RenderCommandDispatcher();
};

}  // namespace Seed

#endif