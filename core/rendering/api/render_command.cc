#include "render_command.h"
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {

void RenderDrawDataBuilder::bind_vertex(RenderResource rc) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::BIND_VERTEX);
    op->vertex_rc = rc;
};
void RenderDrawDataBuilder::bind_index(RenderResource rc) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::BIND_INDEX);
    op->index_rc = rc;
};

void RenderDrawDataBuilder::bind_vertex_data(VertexData &data, u32 offset) {
    bind_vertex(data.get_vertices());
    /* prevent memory allocation */
    if (data.use_index()) {
        bind_index(data.get_indices());
        RenderDrawData *draw_data = get_data();
        draw_data->vertex_cnt = data.get_indices_cnt();
        draw_data->index_offset = offset;
    } else {
        RenderDrawData *draw_data = get_data();
        draw_data->vertex_cnt = data.get_vertices_cnt();
        draw_data->vertex_offset = offset;
    }
}

void RenderDrawDataBuilder::bind_texture(u32 unit, RenderResource rc) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::BIND_TEXTURE);
    op->texure.rc = rc;
    op->texure.unit = unit;
};
void RenderDrawDataBuilder::bind_description(VertexDescription *desc) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::BIND_DESC);
    op->vertex_desc = desc;
};
void RenderDrawDataBuilder::update_constant(RenderResource rc, u32 offset,
                                            u32 size, void *data) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::UPDATE_CONSTANT);
    op->constant.rc = rc;
    op->constant.offset = offset;
    op->constant.size = size;
    op->constant.data =
        RenderEngine::get_instance()->get_mem_pool()->alloc_data(size, data);
};
void RenderDrawDataBuilder::set_viewport(f32 x, f32 y, f32 width, f32 height) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::VIEWPORT);
    op->view_rect = {.x = x, .y = y, .w = width, .h = height};
};
void RenderDrawDataBuilder::set_scissor(f32 x, f32 y, f32 width, f32 height) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::SCISSOR);
    op->scissor_rect = {.x = x, .y = y, .w = width, .h = height};
};

void RenderDrawDataBuilder::set_draw_vertex(u32 vertex_cnt, u32 vertex_offset,
                                            u32 instance_cnt) {
    RenderDrawData *data = this->get_data();
    data->vertex_cnt = vertex_cnt;
    data->vertex_offset = vertex_offset;
    data->instance_cnt = instance_cnt;
}
void RenderDrawDataBuilder::set_draw_index(u32 index_cnt, u32 index_offset,
                                           u32 instance_cnt) {
    RenderDrawData *data = this->get_data();
    data->vertex_cnt = index_cnt;
    data->index_offset = index_offset;
    data->instance_cnt = instance_cnt;
}
void RenderStateDataBuilder::bind_framebuffer(RenderResource rc) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_FRAME_BUFFER);
    op->fbo_rc = rc;
}
void RenderStateDataBuilder::bind_render_target(u32 slot,
                                                RenderResource texture,
                                                u32 face) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_RENDER_TARGET);
    op->render_target = {.slot = slot, .face = face, .texture = texture};
}
void RenderStateDataBuilder::bind_depth_stencil_target(u32 slot,
                                                       RenderResource texture,
                                                       u32 face) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_DEPTH_STENCIL_TARGET);
    op->render_target = {.slot = slot, .face = face, .texture = texture};
}
void RenderStateDataBuilder::set_viewport(f32 x, f32 y, f32 width, f32 height) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::VIEWPORT);
    op->view_rect = {x, y, width, height};
}
void RenderStateDataBuilder::set_scissor(f32 x, f32 y, f32 width, f32 height) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::SCISSOR);
    op->scissor_rect = {x, y, width, height};
}
void RenderStateDataBuilder::clear(StateClearFlag flag) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::CLEAR);
    op->clear_flag |= flag;
}

u32 RenderCommandDispatcher::gen_sort_key(f32 depth) {
    u64 sort_key = 0;
    if (depth < 0.0f) depth = 0.0f;
    if (depth > 1.0f) depth = 1.0f;
    u16 depth_value = (u16)((f32)(u16)(-1) * depth);
    sort_key |= ((u32)this->layer & 0b1111111u) << 16;
    sort_key |= (u32)depth_value;
    return sort_key;
}
void RenderCommandDispatcher::set_scope(const std::string &scope) {
    this->scope = scope;
}

RenderCommand RenderCommandDispatcher::prepare_update_cmd(f32 depth) {
    RenderCommand cmd;
    cmd.scope = scope;
    cmd.sort_key = gen_sort_key(depth);
    cmd.type = RenderCommandType::UPDATE;
    cmd.data = RenderEngine::get_instance()
                   ->get_mem_pool()
                   ->alloc_new<RenderUpdateData>();
    return cmd;
}

void RenderCommandDispatcher::update_buffer(RenderResource &buffer, u32 offset,
                                            u32 size, void *data, f32 depth) {
    if (buffer.type != RenderResourceType::VERTEX &&
        buffer.type != RenderResourceType::CONSTANT &&
        buffer.type != RenderResourceType::INDEX)
        return;
    RenderCommand cmd = prepare_update_cmd(depth);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc_data(size, data);

    update_data->rc = buffer;
    update_data->buffer.size = size;
    update_data->buffer.offset = offset;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void *RenderCommandDispatcher::map_buffer(RenderResource &buffer, u32 offset,
                                          u32 size, f32 depth) {
    if (buffer.type != RenderResourceType::VERTEX &&
        buffer.type != RenderResourceType::CONSTANT &&
        buffer.type != RenderResourceType::INDEX)
        return nullptr;
    RenderCommand cmd = prepare_update_cmd(depth);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc(size);

    update_data->rc = buffer;
    update_data->buffer.size = size;
    update_data->buffer.offset = offset;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    return update_data->data;
}

void RenderCommandDispatcher::update_texture(RenderResource &texture, u32 x_off,
                                             u32 y_off, u32 w, u32 h,
                                             void *data, f32 depth) {
    if (texture.type != RenderResourceType::TEXTURE) return;
    RenderCommand cmd = prepare_update_cmd(depth);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc_data(w * h * 4,
                                                                 data);
    update_data->rc = texture;
    update_data->texture.x_off = x_off;
    update_data->texture.y_off = y_off;
    update_data->texture.w = w;
    update_data->texture.h = h;

    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void *RenderCommandDispatcher::map_texture(RenderResource &texture, u32 x_off,
                                           u32 y_off, u32 w, u32 h, f32 depth) {
    if (texture.type != RenderResourceType::TEXTURE) return nullptr;
    RenderCommand cmd = prepare_update_cmd(depth);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc(w * h);
    update_data->rc = texture;
    update_data->texture.x_off = x_off;
    update_data->texture.y_off = y_off;
    update_data->texture.w = w;
    update_data->texture.w = h;

    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    return update_data->data;
}

void RenderCommandDispatcher::update_cubemap(RenderResource &texture, u8 face,
                                             u16 x_off, u16 y_off, u16 w, u16 h,
                                             void *data, f32 depth) {
    if (texture.type != RenderResourceType::TEXTURE) return;
    if (face >= 6) {
        SPDLOG_ERROR("Face is invalid.");
        return;
    }
    RenderCommand cmd = prepare_update_cmd(depth);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc_data(w * h * 4,
                                                                 data);
    update_data->rc = texture;
    update_data->texture.x_off = x_off;
    update_data->texture.y_off = y_off;
    update_data->texture.w = w;
    update_data->texture.h = h;
    update_data->texture.face = face;

    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

RenderDrawDataBuilder RenderCommandDispatcher::generate_render_data(
    Ref<Material> mat) {
    RenderDrawDataBuilder builder;
    if (mat.is_valid()) {
        mat->bind_states(builder);
    }
    return builder;
}

void RenderCommandDispatcher::set_states(RenderStateDataBuilder &builder, f32 depth) {
    RenderStateData *state_data =
        (RenderStateData *)RenderEngine::get_instance()
            ->get_mem_pool()
            ->alloc_data(builder.buffer.size(), builder.buffer.data());
    RenderCommand cmd;
    cmd.scope = scope;
    cmd.sort_key = gen_sort_key(depth);
    cmd.type = RenderCommandType::STATE;
    cmd.data = state_data;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void RenderCommandDispatcher::render(RenderDrawDataBuilder &builder,
                                     RenderPrimitiveType type,
                                     RenderResource pipeline, f32 depth) {
    RenderDrawData *draw_data =
        (RenderDrawData *)RenderEngine::get_instance()
            ->get_mem_pool()
            ->alloc_data(builder.buffer.size(), builder.buffer.data());
    RenderCommand cmd;
    cmd.scope = scope;
    cmd.sort_key = gen_sort_key(depth);
    cmd.type = RenderCommandType::RENDER;
    cmd.data = draw_data;
    draw_data->type = type;
    draw_data->pipeline = pipeline;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

RenderCommandDispatcher::~RenderCommandDispatcher() {
    if (this->start_draw) {
        SPDLOG_WARN("Starting a state without ever ending");
        return;
    }
}

}  // namespace Seed