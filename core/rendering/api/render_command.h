#ifndef _SEED_RENDERING_COMMAND_H_
#define _SEED_RENDERING_COMMAND_H_
#include "render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/rendering/vertex_desc.h"
#include "core/resource/material.h"
#include "core/rendering/api/render_pipeline.h"
#include <queue>
#include <stack>
#include <fmt/format.h>

namespace Seed {
class RenderEngine;
enum class RenderCommandType : u8 { STATE, UPDATE, RENDER };
#define DEBUG_DISPATCH(_dp) _dp.set_scope(fmt::format("DEBUG: {}:{}", __FILE__, __LINE__))
/*
 sort key structure
|--------------------------------------------|
|  unused  |  layer  |  depth  | material id |
|--------------------------------------------|
|  25 bits | 7 bits  | 16 bits |   16 bits   |
|--------------------------------------------|
*/

struct RenderCommand {
        u32 layer;
        RenderCommandType type;
        void *data;
        std::string scope;
        static bool cmp(RenderCommand const &a, RenderCommand const &b) {
            return a.layer < b.layer;
        }
};

struct ViewportState {
        f32 x, y, w, h;
};
struct ScissorRect {
        f32 x, y, w, h;
};

struct RenderDrawData {
        u64 sort_key;
        VertexData *vertices;
        Ref<Material> mat;
        RenderResource instance;
        u32 instance_cnt = 0;
        u32 index_cnt = 0;
        u32 index_offset = 0;
        u32 vertex_offset = 0;
        Ref<RenderPipeline> pipeline;
        bool set_viewport = false;
        bool set_scissor = false;
        ViewportState viewport;
        ScissorRect scissor;
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

class RenderCommandDispatcher {
    private:
        u8 layer = 0;
        bool start_draw = 0;
        u64 draw_key = 0;
        ViewportState viewport;
        std::vector<RenderDrawData *> ordered_draw_data;
        std::string scope;
        inline void ensure_draw_begin();
        RenderCommand prepare_state_cmd();
        RenderCommand prepare_update_cmd();

    public:
        u64 gen_sort_key(f32 depth, u16 material_id);
        u64 gen_sort_key(f32 depth, RenderDrawData &data);
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
        void update_cubemap(RenderResource &texture, u8 face, u16 x_off, u16 y_off,
                            u16 w, u16 h, void *data);

        RenderDrawData generate_render_data(VertexData &vertices,
                                            Ref<Material> mat);
        RenderDrawData generate_render_data(VertexData &vertices,
                                            Ref<Material> mat,
                                            RenderResource &instance,
                                            u32 instance_cnt);
        void begin_draw();

        void draw_set_viewport(RenderDrawData &rdd, f32 x, f32 y, f32 width,
                               f32 height);
        void draw_set_scissor(RenderDrawData &rdd, f32 x, f32 y, f32 width,
                              f32 height);
        void draw_cancel_scissor(RenderDrawData &rdd);

        void render(RenderDrawData *data, Ref<RenderPipeline> pipeline,
                    f32 depth);
        void render(RenderDrawData *data, Ref<RenderPipeline> pipeline,
                    f32 depth, u32 index_cnt, u32 index_offset);
        void end_draw();

        RenderCommandDispatcher(u8 layer) : layer(layer) {}
        ~RenderCommandDispatcher();
};

}  // namespace Seed

#endif