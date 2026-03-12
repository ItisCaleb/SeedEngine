#ifndef _SEED_RENDERING_COMMAND_H_
#define _SEED_RENDERING_COMMAND_H_
#include "render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/rendering/vertex_layout.h"
#include "core/rendering/render_common.h"
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
            BIND_CONSTANT,
            PUSH_CONSTANT,
            VIEWPORT,
            SCISSOR
        };
        struct Operation {
                OpType type;
                union {
                        VertexHandle vertex_handle;
                        IndexHandle index_handle;
                        ConstantHandle constant_handle;
                        struct {
                                u32 unit;
                                TextureHandle texture_handle;
                        } texture;
                        struct {
                                void *data;
                                u32 size;
                        } push_constant;
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
        PipelineHandle pipeline;
        RenderPrimitiveType type;
        bool draw_depth_only = false;
        bool depth_write = false;
        bool depth_clamp = false;
        CompareOP depth_test_op = CompareOP::NEVER;
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
        std::vector<typename T::Operation *> op_view;
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
                op_view[i] =
                    (typename T::Operation *)((u64)data + sizeof(T) +
                                              i * sizeof(
                                                      typename T::Operation));
            }
            return dst;
        }

    public:
        T *get_data() { return static_cast<T *>((void *)&this->buffer[0]); }
        void rollback() {
            T *data = static_cast<T *>((void *)&this->buffer[0]);
            if (data->operation_cnt == 0) return;
            data->operation_cnt--;
            this->buffer.resize(this->buffer.size() -
                                sizeof(typename T::Operation));
        }
        void reset() {
            this->buffer.clear();
            this->buffer.resize(sizeof(T));
            op_view.clear();
        }
        DataBuilder() { this->buffer.resize(sizeof(T)); }
};

class RenderCommandDispatcher;
class RenderDrawDataBuilder : public DataBuilder<RenderDrawData> {
        friend RenderCommandDispatcher;

    public:
        void bind_vertex(VertexHandle handle);
        void bind_index(IndexHandle handle);
        void bind_vertex_data(Ref<VertexData> data, u32 offset = 0);
        void bind_index_data(Ref<IndexData> data, u32 offset = 0);

        void bind_texture(u32 unit, TextureHandle handle);
        void bind_description(VertexLayout *desc);
        void push_constant(u32 size, void *data);

        template <typename T>
        void push_constant(T data) {
            this->push_constant(sizeof(T), &data);
        }

        void set_viewport(f32 x, f32 y, f32 width, f32 height);
        void set_scissor(f32 x, f32 y, f32 width, f32 height);
        void set_draw_vertex(u32 vertex_cnt, u32 vertex_offset);
        void set_draw_index(u32 index_cnt, u32 index_offset);
        void set_instance(u32 cnt, u32 instance_offset = 0);
        void set_draw_depth_only(bool depth_only);
        void set_depth_write(bool depth_write);
        void set_depth_test(CompareOP compare);
        void set_depth_clamp(bool depth_clamp);
};

typedef u32 StateClearFlag;
enum StateClearFlagBits : u32 {
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
            BIND_CONSTANT,
            BIND_STORAGE_BUFFER
        };
        struct Operation {
                OpType type;
                union {
                        RenderPassHandle render_pass_handle;
                        struct {
                                RectF *view_rects;
                                u32 counts;
                        } viewports;
                        RectF scissor_rect;
                        u8 clear_flag;
                        struct {
                                ConstantHandle handle;
                                u32 base;
                        } constant;
                        struct {
                                SSBOHandle handle;
                                u32 base;
                        } ssbo;
                };
        };
        u32 operation_cnt = 0;
};

class RenderStateDataBuilder : public DataBuilder<RenderStateData> {
        friend RenderCommandDispatcher;

    public:
        void bind_render_pass(RenderPassHandle handle);
        void set_viewport(Viewport *viewport, bool flip_y = false);
        void set_viewports(std::vector<Viewport> &viewports,
                           bool flip_y = false);

        void set_scissor(f32 x, f32 y, f32 width, f32 height);
        void set_scissor(const RectF &scissor_rect);
        void set_scissor(Viewport *viewport, bool flip_y = false);
        void bind_constant(ConstantHandle constant, u32 base);
        void bind_storage_buffer(SSBOHandle constant, u32 base);

        void clear(StateClearFlag flag);
};

struct RenderStreamData {
        RenderResourceType type;
        Handle handle;
        u32 size;
        void *get_buffer() {
            return (void *)((u64)this + sizeof(RenderStreamData));
        }
};

class Material;
class RenderCommandQueue;
class RenderCommandDispatcher {
        friend RenderDrawDataBuilder;

    private:
        u8 layer = 0;
        u8 seq = 0;
        void *push_update_cmd(RenderStreamData &update_data, u64 size,
                              void *data);

    public:
        void set_layer(u8 layer) { this->layer = layer; }
        void set_seq(u8 seq) { this->seq = seq; }
        void begin_scope(const std::string &scope);
        void end_scope();

        void set_states(RenderStateDataBuilder &builder);
        /* Will copy data to a temporary buffer.*/
        void push_buffer(VertexHandle handle, u32 size, void *data);
        void push_buffer(IndexHandle handle, u32 size, void *data);
        void push_buffer(ConstantHandle handle, u32 size, void *data);
        void push_buffer(SSBOHandle handled, u32 size, void *data);

        /* will automatically fill material state and textures */
        RenderDrawDataBuilder generate_render_data(Ref<Material> mat);

        void render(RenderDrawDataBuilder &builder, RenderPrimitiveType type,
                    PipelineHandle pipeline, f32 depth);

        RenderCommandDispatcher();
        ~RenderCommandDispatcher();
};

}  // namespace Seed

#endif