#include "render_command.h"
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {

u64 RenderCommandDispatcher::gen_sort_key(f32 depth, u16 material_id) {
    u64 sort_key = 0;
    if (depth < 0.0f) depth = 0.0f;
    if (depth > 1.0f) depth = 1.0f;
    u16 depth_value = (u16)((f32)(u16)(-1) * depth);
    sort_key |= ((u64)this->layer & 0b1111111u) << 32;
    sort_key |= (u64)depth_value << 16;
    sort_key |= (u64)material_id;
    return sort_key;
}
u64 RenderCommandDispatcher::gen_sort_key(f32 depth, RenderDrawData &data) {
    u16 mat_id = data.mat.is_null() ? 0 : data.mat->get_id();
    return gen_sort_key(depth, mat_id);
}

void RenderCommandDispatcher::clear(StateClearFlag flag) {
    RenderCommand cmd;
    cmd.layer = layer;
    cmd.type = RenderCommandType::STATE;
    RenderStateData *state_data =
        (RenderStateData *)malloc(sizeof(RenderStateData));
    cmd.data = state_data;
    state_data->flag |= RenderStateFlag::CLEAR;
    state_data->clear_flag |= flag;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void RenderCommandDispatcher::set_viewport(f32 x, f32 y, f32 width,
                                           f32 height) {
    RenderCommand cmd;
    cmd.layer = layer;
    cmd.type = RenderCommandType::STATE;
    RenderStateData *state_data =
        (RenderStateData *)malloc(sizeof(RenderStateData));
    cmd.data = state_data;
    state_data->flag |= RenderStateFlag::VIEWPORT;
    state_data->viewport = {.x = x, .y = y, .w = width, .h = height};
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}
void RenderCommandDispatcher::set_scissor(f32 x, f32 y, f32 width, f32 height) {
    RenderCommand cmd;
    cmd.layer = layer;
    cmd.type = RenderCommandType::STATE;
    RenderStateData *state_data =
        (RenderStateData *)malloc(sizeof(RenderStateData));
    cmd.data = state_data;
    state_data->flag |= RenderStateFlag::SCISSOR;
    state_data->rect = {.x = x, .y = y, .w = width, .h = height};
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}
void RenderCommandDispatcher::cancel_scissor() {
    this->set_scissor(this->viewport.x, this->viewport.y, this->viewport.w,
                      this->viewport.h);
}

void RenderCommandDispatcher::update_buffer(RenderResource *buffer, u32 offset,
                                            u32 size, void *data) {
    if (buffer->type != RenderResourceType::VERTEX &&
        buffer->type != RenderResourceType::CONSTANT &&
        buffer->type != RenderResourceType::INDEX)
        return;
    RenderCommand cmd;

    cmd.layer = layer;

    cmd.type = RenderCommandType::UPDATE;
    RenderUpdateData *update_data =
        (RenderUpdateData *)malloc(sizeof(RenderUpdateData));
    cmd.data = update_data;

    update_data->data = data;

    update_data->dst_buffer = buffer;
    update_data->buffer.size = size;
    update_data->buffer.offset = offset;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void *RenderCommandDispatcher::map_buffer(RenderResource *buffer, u32 offset,
                                          u32 size) {
    if (buffer->type != RenderResourceType::VERTEX &&
        buffer->type != RenderResourceType::CONSTANT &&
        buffer->type != RenderResourceType::INDEX)
        return nullptr;
    RenderCommand cmd;
    cmd.layer = layer;

    cmd.type = RenderCommandType::UPDATE;
    RenderUpdateData *update_data =
        (RenderUpdateData *)malloc(sizeof(RenderUpdateData));
    cmd.data = update_data;

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc(size);

    update_data->dst_buffer = buffer;
    update_data->buffer.size = size;
    update_data->buffer.offset = offset;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    return update_data->data;
}

void RenderCommandDispatcher::update_texture(RenderResource *texture, u16 x_off,
                                             u16 y_off, u16 w, u16 h,
                                             void *data) {
    if (texture->type != RenderResourceType::TEXTURE) return;
    RenderCommand cmd;
    cmd.layer = layer;

    cmd.type = RenderCommandType::UPDATE;
    RenderUpdateData *update_data =
        (RenderUpdateData *)malloc(sizeof(RenderUpdateData));
    cmd.data = update_data;

    update_data->data = data;
    update_data->dst_buffer = texture;
    update_data->texture.x_off = x_off;
    update_data->texture.y_off = y_off;
    update_data->texture.w = w;
    update_data->texture.w = h;

    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void *RenderCommandDispatcher::map_texture(RenderResource *texture, u16 x_off,
                                           u16 y_off, u16 w, u16 h) {
    if (texture->type != RenderResourceType::TEXTURE) return nullptr;
    RenderCommand cmd;
    cmd.layer = layer;

    cmd.type = RenderCommandType::UPDATE;
    RenderUpdateData *update_data =
        (RenderUpdateData *)malloc(sizeof(RenderUpdateData));
    cmd.data = update_data;

    update_data->data =
        RenderEngine::get_instance()->get_mem_pool()->alloc(w * h);
    update_data->dst_buffer = texture;
    update_data->texture.x_off = x_off;
    update_data->texture.y_off = y_off;
    update_data->texture.w = w;
    update_data->texture.w = h;

    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    return update_data->data;
}

RenderDrawData RenderCommandDispatcher::generate_render_data(
    VertexData *vertices, Ref<Material> mat) {
    RenderDrawData dispatch_data;
    if (!vertices || !vertices->get_vertices()->inited()) {
        SPDLOG_WARN("Vertices is null or uninited.");
    }
    dispatch_data.vertices = vertices;
    dispatch_data.mat = mat;
    dispatch_data.index_cnt = vertices->get_indices_cnt();
    return dispatch_data;
}

RenderDrawData RenderCommandDispatcher::generate_render_data(
    VertexData *vertices, Ref<Material> mat, RenderResource *instance,
    u32 instance_cnt) {
    RenderDrawData dispatch_data;
    if (!vertices || !vertices->get_vertices()->inited()) {
        SPDLOG_WARN("Vertices is null or uninited.");
    }
    dispatch_data.vertices = vertices;
    dispatch_data.mat = mat;
    dispatch_data.instance = instance;
    dispatch_data.instance_cnt = instance_cnt;
    dispatch_data.index_cnt = vertices->get_indices_cnt();

    return dispatch_data;
}

void RenderCommandDispatcher::ensure_draw_begin() {
    if (!this->start_draw) {
        throw std::runtime_error("Ending a state without ever starting");
    }
}

void RenderCommandDispatcher::begin_draw() {
    if (this->start_draw) {
        SPDLOG_ERROR("Starting a state without ever ending");
        return;
    }
    this->start_draw = true;
}

void RenderCommandDispatcher::draw_set_viewport(RenderDrawData &rdd, f32 x,
                                                f32 y, f32 width, f32 height) {
    ensure_draw_begin();
    rdd.set_viewport = true;
    rdd.viewport = {.x = x, .y = y, .w = width, .h = height};
    this->viewport = rdd.viewport;
}

void RenderCommandDispatcher::end_draw() {
    ensure_draw_begin();
    for (RenderDrawData *rdd : this->ordered_draw_data) {
        RenderCommand cmd;
        cmd.layer = layer;
        cmd.type = RenderCommandType::RENDER;
        cmd.data = rdd;
        RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    }
    this->ordered_draw_data.clear();
    start_draw = false;
}
void RenderCommandDispatcher::draw_set_scissor(RenderDrawData &rdd, f32 x,
                                               f32 y, f32 width, f32 height) {
    ensure_draw_begin();
    rdd.set_scissor = true;
    rdd.scissor = {.x = x, .y = y, .w = width, .h = height};
}
void RenderCommandDispatcher::draw_cancel_scissor(RenderDrawData &rdd) {
    ensure_draw_begin();
    this->draw_set_scissor(rdd, this->viewport.x, this->viewport.y,
                           this->viewport.w, this->viewport.h);
}

void RenderCommandDispatcher::render(RenderDrawData *data,
                                     Ref<RenderPipeline> pipeline, f32 depth) {
    ensure_draw_begin();
    if (data == nullptr) {
        SPDLOG_ERROR("Render data is null.");
        return;
    }

    RenderDrawData *draw_data =
        (RenderDrawData *)malloc(sizeof(RenderDrawData));
    *draw_data = *data;
    draw_data->pipeline = pipeline;
    ordered_draw_data.push_back(draw_data);
}

void RenderCommandDispatcher::render(RenderDrawData *data,
                                     Ref<RenderPipeline> pipeline, f32 depth,
                                     u32 index_cnt, u32 index_offset) {
    ensure_draw_begin();
    if (data == nullptr) {
        SPDLOG_ERROR("Render data is null.");
        return;
    }

    RenderDrawData *draw_data =
        (RenderDrawData *)malloc(sizeof(RenderDrawData));
    *draw_data = *data;
    draw_data->pipeline = pipeline;
    draw_data->index_offset = index_offset;
    draw_data->index_cnt = index_cnt;
    ordered_draw_data.push_back(draw_data);
}

RenderCommandDispatcher::~RenderCommandDispatcher() {
    if (this->start_draw) {
        SPDLOG_WARN("Starting a state without ever ending");
        return;
    }
}

}  // namespace Seed