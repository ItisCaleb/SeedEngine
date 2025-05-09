#include "render_command.h"
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {

u64 RenderCommandDispatcher::gen_sort_key(RenderCommandType type, u16 depth,
                                          u16 material_id) {
    u64 sort_key = 0;
    u8 type_num = static_cast<u8>(type);
    sort_key |= ((u64)this->layer & 0b1111111u) << 34;
    sort_key |= ((u64)type_num & 0b11u) << 18;
    if (type == RenderCommandType::RENDER) {
        sort_key |= (u64)depth << 16;
        sort_key |= (u64)material_id;
    }
    return sort_key;
}
void RenderCommandDispatcher::begin_state() {
    // if (this->beginned()) {
    //     SPDLOG_WARN("Starting a state without ever ending");
    // }
}

void RenderCommandDispatcher::end_state() {
    // if (!this->beginned()) {
    //     SPDLOG_WARN("Ending a state without ever starting");
    // }
    // this->current_layer = -1;
}
void RenderCommandDispatcher::update_buffer(RenderResource *buffer, u32 offset,
                                            u32 size, void *data) {
    if (buffer->type != RenderResourceType::VERTEX &&
        buffer->type != RenderResourceType::CONSTANT &&
        buffer->type != RenderResourceType::INDEX)
        return;
    RenderCommand cmd;
    cmd.sort_key = gen_sort_key(RenderCommandType::UPDATE, 0, 0);
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
    cmd.sort_key = gen_sort_key(RenderCommandType::UPDATE, 0, 0);
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
    cmd.sort_key = gen_sort_key(RenderCommandType::UPDATE, 0, 0);
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
    cmd.sort_key = gen_sort_key(RenderCommandType::UPDATE, 0, 0);
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

RenderDispatchData RenderCommandDispatcher::generate_render_data(
    VertexData *vertices, VertexDescription *desc,
    RenderPrimitiveType prim_type, Ref<Material> mat) {
    RenderDispatchData dispatch_data;
    if (!vertices || !vertices->get_vertices()->inited()) {
        SPDLOG_WARN("Vertices is null or uninited.");
    }
    dispatch_data.vertices = vertices;
    dispatch_data.desc = desc;
    dispatch_data.prim_type = prim_type;
    dispatch_data.mat = mat;
    return dispatch_data;
}

RenderDispatchData RenderCommandDispatcher::generate_render_data(
    VertexData *vertices, VertexDescription *desc,
    RenderPrimitiveType prim_type, Ref<Material> mat, RenderResource *instance,
    VertexDescription *instance_desc, u32 instance_cnt) {
    RenderDispatchData dispatch_data;
    if (!vertices || !vertices->get_vertices()->inited()) {
        SPDLOG_WARN("Vertices is null or uninited.");
    }
    dispatch_data.vertices = vertices;
    dispatch_data.desc = desc;
    dispatch_data.prim_type = prim_type;
    dispatch_data.mat = mat;
    dispatch_data.instance = instance;
    dispatch_data.instance_desc = instance_desc;
    dispatch_data.instance_cnt = instance_cnt;
    return dispatch_data;
}

void RenderCommandDispatcher::render(RenderDispatchData *data,
                                     RenderResource *shader) {
    if (data == nullptr) {
        SPDLOG_ERROR("Render data is null.");
        return;
    }
    RenderCommand cmd;
    u16 mat_id = data->mat.is_null() ? 0 : data->mat->get_id();
    cmd.sort_key = gen_sort_key(RenderCommandType::RENDER, 0, mat_id);
    RenderDispatchData *dispatch_data =
        (RenderDispatchData *)malloc(sizeof(RenderDispatchData));
    *dispatch_data = *data;
    if (shader->type != RenderResourceType::SHADER) {
        SPDLOG_ERROR("The render resource is not a shader.");
        return;
    }
    cmd.type = RenderCommandType::RENDER;
    cmd.data = dispatch_data;
    dispatch_data->shader = shader;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

}  // namespace Seed