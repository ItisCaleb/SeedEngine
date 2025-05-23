#ifndef _SEED_RENDERING_COMMAND_H_
#define _SEED_RENDERING_COMMAND_H_
#include "render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/rendering/vertex_desc.h"
#include "core/resource/material.h"
#include "core/rendering/render_common.h"
#include "core/collision/shape.h"
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

struct RenderDrawData {
        enum class OpType : u8 {
            BIND_VERTEX,
            BIND_DESC,
            BIND_INDEX,
            BIND_TEXTURE,
            UPDATE_CONSTANT,
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
                        VertexDescription *vertex_desc;
                        RectF view_rect;
                        RectF scissor_rect;
                };
        };

        u32 instance_cnt = 0;
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
            this->buffer.resize(this->buffer.size() + sizeof(typename T::Operation));
            typename T::Operation *dst =
                (typename T::Operation *)(this->buffer.data() +
                                          this->buffer.size() -
                                          sizeof(typename T::Operation));
            dst->type = type;
            T *data = static_cast<T *>((void*)&this->buffer[0]);
            data->operation_cnt++;

            /* for debug */
            op_view.resize(data->operation_cnt);
            for (i32 i = 0; i < data->operation_cnt; i++) {
                op_view[i] =
                    (T *)((u64)data + sizeof(T) + i * sizeof(typename T::Operation));
            }
            return dst;
        }

    public:
        T *get_data() { return static_cast<T *>((void*)&this->buffer[0]); }
        DataBuilder() { this->buffer.resize(sizeof(T)); }
};

class RenderCommandDispatcher;
class RenderDrawDataBuilder : public DataBuilder<RenderDrawData> {
        friend RenderCommandDispatcher;

    public:
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
};

enum StateClearFlag : u8 {
    CLEAR_COLOR = 1,
    CLEAR_DEPTH = 2,
    CLEAR_STENCIL = 4
};

struct RenderStateData {
        enum class OpType : u8 {
            BIND_FRAME_BUFFER,
            BIND_RENDER_TARGET,
            BIND_DEPTH_STENCIL_TARGET,
            VIEWPORT,
            SCISSOR,
            CLEAR
        };
        struct Operation {
                OpType type;
                union {
                        RenderResource fbo_rc;
                        struct {
                                u32 slot;
                                u32 face;
                                RenderResource texture;
                        } render_target;
                        RectF view_rect;
                        RectF scissor_rect;
                        u8 clear_flag;
                };
        };
        u32 operation_cnt = 0;
};

class RenderStateDataBuilder : public DataBuilder<RenderStateData> {
        friend RenderCommandDispatcher;

    public:
        void bind_framebuffer(RenderResource rc);
        void bind_render_target(u32 slot, RenderResource texture, u32 face = 0);
        void bind_depth_stencil_target(u32 slot, RenderResource texture,
                                       u32 face = 0);
        void set_viewport(f32 x, f32 y, f32 width, f32 height);
        void set_scissor(f32 x, f32 y, f32 width, f32 height);
        void clear(StateClearFlag flag);
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
                        u32 x_off;
                        u32 y_off;
                        u32 w;
                        u32 h;
                        u8 face;
                } texture;
        };
};

class RenderCommandDispatcher {
    private:
        u8 layer = 0;
        bool start_draw = 0;
        RectF viewport;
        std::string scope;
        RenderCommand prepare_update_cmd(f32 depth);

    public:
        u32 gen_sort_key(f32 depth);
        void set_scope(const std::string &scope);

        void set_states(RenderStateDataBuilder &builder, f32 depth);
        /* Will copy data to a temporary buffer.*/
        void update_buffer(RenderResource &buffer, u32 offset, u32 size,
                           void *data, f32 depth = 0);
        void *map_buffer(RenderResource &buffer, u32 offset, u32 size,
                         f32 depth = 0);

        /* Will copy data to a temporary buffer.*/
        void update_texture(RenderResource &texture, u32 x_off, u32 y_off,
                            u32 w, u32 h, void *data, f32 depth = 0);
        void *map_texture(RenderResource &buffer, u32 x_off, u32 y_off, u32 w,
                          u32 h, f32 depth = 0);
        void update_cubemap(RenderResource &texture, u8 face, u16 x_off,
                            u16 y_off, u16 w, u16 h, void *data, f32 depth = 0);

        /* will automatically fill material state and textures */
        RenderDrawDataBuilder generate_render_data(Ref<Material> mat);

        void render(RenderDrawDataBuilder &builder, RenderPrimitiveType type,
                    RenderResource pipeline, f32 depth);

        RenderCommandDispatcher(u8 layer) : layer(layer) {}
        ~RenderCommandDispatcher();
};

}  // namespace Seed

#endif