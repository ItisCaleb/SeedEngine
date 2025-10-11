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
void RenderDrawDataBuilder::bind_description(VertexLayout *desc) {
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

void RenderDrawDataBuilder::set_instance(u32 cnt){
    this->get_data()->instance_cnt = cnt;
}

void RenderStateDataBuilder::bind_render_target(RenderResource target) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_RENDER_TARGET);
    op->render_target = target;
}

void RenderStateDataBuilder::bind_window() {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_RENDER_TARGET);
    op->render_target = {};
}

void RenderStateDataBuilder::set_viewport(f32 x, f32 y, f32 width, f32 height,
                                          f32 max_height) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::VIEWPORT);
    op->viewport.view_rect = {x, y, width, height};
    op->viewport.max_height = max_height;
}

void RenderStateDataBuilder::set_viewport(const RectF &viewport,
                                          f32 max_height) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::VIEWPORT);
    op->viewport.view_rect = viewport;
    op->viewport.max_height = max_height;
}

void RenderStateDataBuilder::set_scissor(f32 x, f32 y, f32 width, f32 height) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::SCISSOR);
    op->scissor_rect = {x, y, width, height};
}

void RenderStateDataBuilder::set_scissor(const RectF &scissor_rect) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::SCISSOR);
    op->scissor_rect = scissor_rect;
}
void RenderStateDataBuilder::clear(StateClearFlag flag) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::CLEAR);
    op->clear_flag |= flag;
}

void RenderStateDataBuilder::bind_bufferbase(RenderResource buffer, u32 base) {
    if (buffer.type != RenderResourceType::BUFFER) {
        SPDLOG_ERROR("Can't bind a buffer which type isn't 'Buffer'.");
        return;
    }
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_BUFFERBASE);
    op->bufferbase.buffer = buffer;
    op->bufferbase.base = base;
}

void RenderCommandDispatcher::begin_scope(const std::string &scope,
                                          u32 sort_key) {
    this->scope = scope;
    RenderCommand cmd;
    cmd.scope = scope;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::BEGIN_SCOPE;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void RenderCommandDispatcher::end_scope(u32 sort_key) {
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::END_SCOPE;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

RenderCommand RenderCommandDispatcher::prepare_update_cmd(u32 sort_key) {
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::UPDATE;
    cmd.data = RenderEngine::get_instance()->get_mem_pool()->alloc(
        sizeof(RenderUpdateData));
    return cmd;
}

void RenderCommandDispatcher::update_buffer(const RenderResource &buffer, u32 offset,
                                            u32 size, void *data,
                                            u32 sort_key) {
    if (buffer.type != RenderResourceType::VERTEX &&
        buffer.type != RenderResourceType::CONSTANT &&
        buffer.type != RenderResourceType::INDEX &&
        buffer.type != RenderResourceType::BUFFER)
        return;
    RenderCommand cmd = prepare_update_cmd(sort_key);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc_data(size, data);

    update_data->rc = buffer;
    update_data->buffer.size = size;
    update_data->buffer.offset = offset;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void *RenderCommandDispatcher::map_buffer(const RenderResource &buffer, u32 offset,
                                          u32 size, u32 sort_key) {
    if (buffer.type != RenderResourceType::VERTEX &&
        buffer.type != RenderResourceType::CONSTANT &&
        buffer.type != RenderResourceType::INDEX &&
        buffer.type != RenderResourceType::BUFFER)
        return nullptr;
    RenderCommand cmd = prepare_update_cmd(sort_key);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc(size);

    update_data->rc = buffer;
    update_data->buffer.size = size;
    update_data->buffer.offset = offset;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    return update_data->data;
}

void RenderCommandDispatcher::update_texture(const RenderResource &texture, u32 x_off,
                                             u32 y_off, u32 w, u32 h,
                                             void *data, u32 sort_key) {
    if (texture.type != RenderResourceType::TEXTURE) return;
    RenderCommand cmd = prepare_update_cmd(sort_key);
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

void *RenderCommandDispatcher::map_texture(const RenderResource &texture, u32 x_off,
                                           u32 y_off, u32 w, u32 h,
                                           u32 sort_key) {
    if (texture.type != RenderResourceType::TEXTURE) return nullptr;
    RenderCommand cmd = prepare_update_cmd(sort_key);
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

void RenderCommandDispatcher::update_cubemap(const RenderResource &texture, u8 face,
                                             u16 x_off, u16 y_off, u16 w, u16 h,
                                             void *data, u32 sort_key) {
    if (texture.type != RenderResourceType::TEXTURE) return;
    if (face >= 6) {
        SPDLOG_ERROR("Face is invalid.");
        return;
    }
    RenderCommand cmd = prepare_update_cmd(sort_key);
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

void RenderCommandDispatcher::update_color_attachment(
    const RenderResource &render_target, i32 slot, RenderResource tex, u32 face,
    u32 sort_key) {
    if (slot < 0) {
        SPDLOG_ERROR("Can't update attachment with slot smaller than 0");
        return;
    }
    RenderCommand cmd = prepare_update_cmd(sort_key);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);
    update_data->data = nullptr;
    update_data->rc = render_target;
    update_data->attachment.face = face;
    update_data->attachment.slot = slot;
    update_data->attachment.texture = tex;

    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}
void RenderCommandDispatcher::update_depth_attachment(
    const RenderResource &render_target, RenderResource tex, u32 face, u32 sort_key) {
    RenderCommand cmd = prepare_update_cmd(sort_key);
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);

    update_data->data = nullptr;
    update_data->rc = render_target;
    update_data->attachment.face = face;
    update_data->attachment.slot = -1;
    update_data->attachment.texture = tex;
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

void RenderCommandDispatcher::set_states(RenderStateDataBuilder &builder,
                                         u32 sort_key) {
    RenderStateData *state_data =
        (RenderStateData *)RenderEngine::get_instance()
            ->get_mem_pool()
            ->alloc_data(builder.buffer.size(), builder.buffer.data());
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::STATE;
    cmd.data = state_data;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void RenderCommandDispatcher::render(RenderDrawDataBuilder &builder,
                                     RenderPrimitiveType type,
                                     RenderResource pipeline, u32 sort_key) {
    RenderDrawData *draw_data =
        (RenderDrawData *)RenderEngine::get_instance()
            ->get_mem_pool()
            ->alloc_data(builder.buffer.size(), builder.buffer.data());
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::RENDER;
    cmd.data = draw_data;
    draw_data->type = type;
    draw_data->pipeline = pipeline;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

RenderCommandDispatcher::~RenderCommandDispatcher() {}

}  // namespace Seed