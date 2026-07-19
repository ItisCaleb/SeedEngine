#include "render_command.h"
#include "render_engine.h"
#include "core/macro.h"
#include "core/resource/material.h"

namespace Seed {

#define RD RenderEngine::get_instance()->get_device()

void RenderDrawDataBuilder::bind_vertex(VertexHandle handle) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::BIND_VERTEX);
    op->vertex_handle = handle;
};
void RenderDrawDataBuilder::bind_index(IndexHandle handle) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::BIND_INDEX);
    op->index_handle = handle;
};

void RenderDrawDataBuilder::bind_vertex_data(Ref<VertexData> data, u32 offset) {
    EXPECT_NOT_NULL_RET(data.ptr());
    bind_vertex(data->get_handle());
    RenderDrawData *draw_data = get_data();
    draw_data->vertex_cnt = data->get_count();
    draw_data->vertex_offset = offset;
    bind_description(data->get_layout());
}

void RenderDrawDataBuilder::bind_index_data(Ref<IndexData> data, u32 offset) {
    EXPECT_NOT_NULL_RET(data.ptr());
    bind_index(data->get_handle());
    RenderDrawData *draw_data = get_data();
    draw_data->vertex_cnt = data->get_size();
    draw_data->index_offset = offset;
}

void RenderDrawDataBuilder::push_constant(u32 size, void *data) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::PUSH_CONSTANT);
    op->push_constant.data = RD->alloc(size, data);
    op->push_constant.size = size;
}

void RenderDrawDataBuilder::bind_texture(u32 unit, TextureHandle handle) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::BIND_TEXTURE);
    op->texture.texture_handle = handle;
    op->texture.unit = unit;
};

void RenderDrawDataBuilder::bind_constant(u32 unit, ConstantHandle handle) {
    RenderDrawData::Operation *op =
        alloc_operation(RenderDrawData::OpType::BIND_CONSTANT);
    op->constant.constant_handle = handle;
    op->constant.unit = unit;
}

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

void RenderDrawDataBuilder::set_instance(u32 instance_cnt,
                                         u32 instance_offset) {
    RenderDrawData *data = this->get_data();
    data->instance_cnt = instance_cnt;
    data->instance_offset = instance_offset;
}

void RenderDrawDataBuilder::set_draw_depth_only(bool depth_only) {
    RenderDrawData *data = this->get_data();
    data->draw_depth_only = depth_only;
}

void RenderDrawDataBuilder::set_depth_write(bool depth_write) {
    RenderDrawData *data = this->get_data();
    data->depth_write = depth_write;
}

void RenderDrawDataBuilder::set_depth_test(CompareOP compare) {
    RenderDrawData *data = this->get_data();
    data->depth_test_op = compare;
}

void RenderDrawDataBuilder::set_depth_clamp(bool depth_clamp) {
    RenderDrawData *data = this->get_data();
    data->depth_clamp = depth_clamp;
}

void RenderStateDataBuilder::bind_render_pass(RenderPassHandle handle) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_RENDER_TARGET);
    op->render_pass_handle = handle;
}

void RenderStateDataBuilder::set_viewport(Viewport *viewport, bool flip_y) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::VIEWPORT);
    op->viewports.view_rects = (RectF *)RD->alloc(sizeof(RectF));
    *op->viewports.view_rects = viewport->get_actual_dimension(flip_y);
    op->viewports.counts = 1;
}

void RenderStateDataBuilder::set_viewports(std::vector<Viewport> &viewports,
                                           bool flip_y) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::VIEWPORT);
    op->viewports.view_rects =
        (RectF *)RD->alloc(viewports.size() * sizeof(RectF));
    for (u32 i = 0; i < viewports.size(); i++) {
        op->viewports.view_rects[i] = viewports[i].get_actual_dimension(flip_y);
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

void RenderStateDataBuilder::set_scissor(Viewport *viewport, bool flip_y) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::SCISSOR);
    op->scissor_rect = viewport->get_actual_dimension(flip_y);
}

void RenderStateDataBuilder::clear(StateClearFlag flag) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::CLEAR);
    op->clear_flag = flag;
}

void RenderStateDataBuilder::bind_constant(ConstantHandle handle, u32 base) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_CONSTANT);
    op->constant.handle = handle;
    op->constant.base = base;
}

void RenderStateDataBuilder::bind_storage_buffer(SSBOHandle handle, u32 base) {
    RenderStateData::Operation *op =
        alloc_operation(RenderStateData::OpType::BIND_STORAGE_BUFFER);
    op->ssbo.handle = handle;
    op->ssbo.base = base;
}

void RenderCommandDispatcher::begin_scope(const std::string &scope) {
    RenderCommand cmd;
    cmd.sort_key = gen_sort_key(layer, seq, 0);
    cmd.type = RenderCommandType::BEGIN_SCOPE;
    RD->push_cmd(cmd, scope.size() + 1, (void *)scope.c_str());
}

void RenderCommandDispatcher::end_scope() {
    RenderCommand cmd;
    cmd.sort_key = gen_sort_key(layer, seq + 1, 0);
    ;
    cmd.type = RenderCommandType::END_SCOPE;
    RD->push_cmd(cmd);
}

void *RenderCommandDispatcher::push_update_cmd(RenderStreamData &update_data,
                                               u64 size, void *data = nullptr) {
    RenderCommand cmd;
    cmd.sort_key = gen_sort_key(layer, seq, 0);
    cmd.type = RenderCommandType::UPDATE;
    u64 stream_size = sizeof(RenderStreamData) + size;

    RenderStreamData *upd =
        (RenderStreamData *)RD->push_cmd(cmd, stream_size, nullptr);
    *upd = update_data;
    memcpy(upd->get_buffer(), data, size);

    return cmd.data;
}

void RenderCommandDispatcher::push_buffer(VertexHandle handle, u32 size,
                                          void *data) {
    if (size == 0) {
        SEED_WARN("Can't push vertex buffer, size is 0");
        return;
    }
    if (data == nullptr) {
        SEED_WARN("Can't push vertex buffer, data is mullptr");
        return;
    }
    RenderStreamData update_data;
    update_data.type = RenderResourceType::VERTEX;
    update_data.handle = handle;
    update_data.size = size;
    push_update_cmd(update_data, size, data);
}
void RenderCommandDispatcher::push_buffer(IndexHandle handle, u32 size,
                                          void *data) {
    if (size == 0) {
        SEED_WARN("Can't push index buffer, size is 0");
        return;
    }
    if (data == nullptr) {
        SEED_WARN("Can't push index buffer, data is mullptr");
        return;
    }
    RenderStreamData update_data;
    update_data.type = RenderResourceType::INDEX;
    update_data.handle = handle;
    update_data.size = size;
    push_update_cmd(update_data, size, data);
}
void RenderCommandDispatcher::push_buffer(ConstantHandle handle, u32 size,
                                          void *data) {
    if (size == 0) {
        SEED_WARN("Can't push constant buffer, size is 0");
        return;
    }
    if (data == nullptr) {
        SEED_WARN("Can't push constant buffer, data is mullptr");
        return;
    }
    RenderStreamData update_data;
    update_data.type = RenderResourceType::CONSTANT;
    update_data.handle = handle;
    update_data.size = size;
    push_update_cmd(update_data, size, data);
}
void RenderCommandDispatcher::push_buffer(SSBOHandle handle, u32 size,
                                          void *data) {
    if (size == 0) {
        SEED_WARN("Can't push ssbo buffer, size is 0");
        return;
    }
    if (data == nullptr) {
        SEED_WARN("Can't push ssbp buffer, data is mullptr");
        return;
    }
    RenderStreamData update_data;
    update_data.type = RenderResourceType::STORAGE_BUFFER;
    update_data.handle = handle;
    update_data.size = size;
    push_update_cmd(update_data, size, data);
}

RenderDrawDataBuilder RenderCommandDispatcher::generate_render_data(
    Ref<Material> mat) {
    RenderDrawDataBuilder builder;
    if (mat.is_valid()) {
        mat->bind_states(builder);
    }
    return builder;
}

void RenderCommandDispatcher::set_states(RenderStateDataBuilder &builder) {
    RenderCommand cmd;
    cmd.sort_key = gen_sort_key(layer, seq, 0);
    cmd.type = RenderCommandType::STATE;
    RD->push_cmd(cmd, builder.buffer.size(), builder.buffer.data());
}

void RenderCommandDispatcher::render(RenderDrawDataBuilder &builder,
                                     RenderPrimitiveType type,
                                     PipelineHandle pipeline, f32 depth) {
    RenderCommand cmd;
    cmd.sort_key = gen_sort_key(layer, seq, depth);
    cmd.type = RenderCommandType::RENDER;
    RenderDrawData *draw_data = static_cast<RenderDrawData *>(
        RD->push_cmd(cmd, builder.buffer.size(), builder.buffer.data()));
    draw_data->type = type;
    draw_data->pipeline = pipeline;
}

RenderCommandDispatcher::RenderCommandDispatcher() {}
RenderCommandDispatcher::~RenderCommandDispatcher() {}

}  // namespace Seed