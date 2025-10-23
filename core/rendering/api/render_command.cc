#include "render_command.h"
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {

#define RD RenderEngine::get_instance()->get_device()

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

void RenderDrawDataBuilder::bind_vertex_data(Ref<VertexData> data, u32 offset) {
    EXPECT_NOT_NULL_RET(data.ptr());
    bind_vertex(data->get_resource());
    RenderDrawData *draw_data = get_data();
    draw_data->vertex_cnt = data->get_count();
    draw_data->vertex_offset = offset;
    bind_description(data->get_layout());
}

void RenderDrawDataBuilder::bind_index_data(Ref<IndexData> data, u32 offset) {
    EXPECT_NOT_NULL_RET(data.ptr());
    bind_index(data->get_resource());
    RenderDrawData *draw_data = get_data();
    draw_data->vertex_cnt = data->get_size();
    draw_data->index_offset = offset;
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

void RenderDrawDataBuilder::set_draw_vertex(u32 vertex_cnt, u32 vertex_offset) {
    RenderDrawData *data = this->get_data();
    data->vertex_cnt = vertex_cnt;
    data->vertex_offset = vertex_offset;
}
void RenderDrawDataBuilder::set_draw_index(u32 index_cnt, u32 index_offset) {
    RenderDrawData *data = this->get_data();
    data->vertex_cnt = index_cnt;
    data->index_offset = index_offset;
}

void RenderDrawDataBuilder::set_instance(u32 cnt) {
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

void RenderStateDataBuilder::set_viewport(Viewport *viewport) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::VIEWPORT);
    op->viewports.view_rects = (RectF *)malloc(sizeof(RectF));
    *op->viewports.view_rects = viewport->get_actual_dimension(true);
    op->viewports.counts = 1;
}

void RenderStateDataBuilder::set_viewports(std::vector<Viewport> &viewports) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::VIEWPORT);
    op->viewports.view_rects =
        (RectF *)malloc(viewports.size() * sizeof(RectF));
    for (u32 i = 0; i < viewports.size(); i++) {
        op->viewports.view_rects[i] = viewports[i].get_actual_dimension(true);
    }
    op->viewports.counts = viewports.size();
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
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::BEGIN_SCOPE;
    RD->push_cmd(cmd, scope.size() + 1, (void *)scope.c_str());
}

void RenderCommandDispatcher::end_scope(u32 sort_key) {
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::END_SCOPE;
    RD->push_cmd(cmd);
}

void *RenderCommandDispatcher::push_update_cmd(RenderUpdateData &update_data,
                                               u32 sort_key, u64 size,
                                               void *data = nullptr) {
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::UPDATE;
    cmd.data = malloc(sizeof(RenderUpdateData) + size);
    RenderUpdateData *upd = (RenderUpdateData *)cmd.data;
    *upd = update_data;
    if (size > 0) {
        if (data) {
            memcpy(upd->get_buffer(), data, size);
            upd->filled = true;
        } else {
            upd->filled = false;
        }
    } else {
        upd->filled = true;
    }
    RD->push_cmd(cmd);

    return cmd.data;
}

void RenderCommandDispatcher::update_buffer(const RenderResource &buffer,
                                            u32 offset, u32 size, void *data,
                                            u32 sort_key) {
    if (buffer.type != RenderResourceType::VERTEX &&
        buffer.type != RenderResourceType::CONSTANT &&
        buffer.type != RenderResourceType::INDEX &&
        buffer.type != RenderResourceType::BUFFER)
        return;
    if (size == 0) return;
    RenderUpdateData update_data;
    update_data.rc = buffer;
    update_data.buffer.size = size;
    update_data.buffer.offset = offset;
    push_update_cmd(update_data, sort_key, size, data);
}

RenderUpdateData *RenderCommandDispatcher::map_buffer(
    const RenderResource &buffer, u32 offset, u32 size, u32 sort_key) {
    if (buffer.type != RenderResourceType::VERTEX &&
        buffer.type != RenderResourceType::CONSTANT &&
        buffer.type != RenderResourceType::INDEX &&
        buffer.type != RenderResourceType::BUFFER)
        return nullptr;
    if (size == 0) return nullptr;

    RenderUpdateData update_data;
    update_data.rc = buffer;
    update_data.buffer.size = size;
    update_data.buffer.offset = offset;
    return (RenderUpdateData *)push_update_cmd(update_data, sort_key, size);
}

void RenderCommandDispatcher::update_texture(const RenderResource &texture,
                                             u32 x_off, u32 y_off, u32 w, u32 h,
                                             void *data, u32 sort_key) {
    if (texture.type != RenderResourceType::TEXTURE) return;
    if (w == 0 || h == 0) return;
    RenderUpdateData update_data;
    update_data.rc = texture;
    update_data.texture.x_off = x_off;
    update_data.texture.y_off = y_off;
    update_data.texture.w = w;
    update_data.texture.h = h;
    push_update_cmd(update_data, sort_key, w * h * 4, data);
}

RenderUpdateData *RenderCommandDispatcher::map_texture(
    const RenderResource &texture, u32 x_off, u32 y_off, u32 w, u32 h,
    u32 sort_key) {
    if (texture.type != RenderResourceType::TEXTURE) return nullptr;
    if (w == 0 || h == 0) return nullptr;

    RenderUpdateData update_data;
    update_data.rc = texture;
    update_data.texture.x_off = x_off;
    update_data.texture.y_off = y_off;
    update_data.texture.w = w;
    update_data.texture.h = h;
    return (RenderUpdateData *)push_update_cmd(update_data, sort_key,
                                               w * h * 4);
}

void RenderCommandDispatcher::update_cubemap(const RenderResource &texture,
                                             u8 face, u16 x_off, u16 y_off,
                                             u16 w, u16 h, void *data,
                                             u32 sort_key) {
    if (texture.type != RenderResourceType::TEXTURE) return;
    if (face >= 6) {
        SPDLOG_ERROR("Face is invalid.");
        return;
    }
    if (w == 0 || h == 0) return;

    RenderUpdateData update_data;
    update_data.rc = texture;
    update_data.texture.x_off = x_off;
    update_data.texture.y_off = y_off;
    update_data.texture.w = w;
    update_data.texture.h = h;
    update_data.texture.face = face;
    push_update_cmd(update_data, sort_key, w * h * 4, data);
}

void RenderCommandDispatcher::update_color_attachment(
    const RenderResource &render_target, i32 slot, RenderResource tex, u32 face,
    u32 sort_key) {
    if (slot < 0) {
        SPDLOG_ERROR("Can't update attachment with slot smaller than 0");
        return;
    }
    RenderUpdateData update_data;
    update_data.rc = render_target;
    update_data.attachment.face = face;
    update_data.attachment.slot = slot;
    update_data.attachment.texture = tex;
    push_update_cmd(update_data, sort_key, 0);
}
void RenderCommandDispatcher::update_depth_attachment(
    const RenderResource &render_target, RenderResource tex, u32 face,
    u32 sort_key) {
    RenderUpdateData update_data;
    update_data.rc = render_target;
    update_data.attachment.face = face;
    update_data.attachment.slot = -1;
    update_data.attachment.texture = tex;
    push_update_cmd(update_data, sort_key, 0);
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
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::STATE;
    RD->push_cmd(cmd, builder.buffer.size(), builder.buffer.data());
}

void RenderCommandDispatcher::render(RenderDrawDataBuilder &builder,
                                     RenderPrimitiveType type,
                                     RenderResource pipeline, u32 sort_key) {
    RenderCommand cmd;
    cmd.sort_key = sort_key;
    cmd.type = RenderCommandType::RENDER;
    RenderDrawData *draw_data = static_cast<RenderDrawData *>(
        RD->push_cmd(cmd, builder.buffer.size(), builder.buffer.data()));
    draw_data->type = type;
    draw_data->pipeline = pipeline;
}

RenderCommandDispatcher::RenderCommandDispatcher() {}
RenderCommandDispatcher::~RenderCommandDispatcher() {}

}  // namespace Seed