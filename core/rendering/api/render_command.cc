#include "render_command.h"
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {

u64 RenderCommandDispatcher::get_sort_key() {
    if (sort_keys.empty()) {
        SPDLOG_WARN("Doesn't set sort_key");
        return 0;

    } else
        return sort_keys.top();
}
void RenderCommandDispatcher::begin(u64 sortkey) {
    this->sort_keys.push(sortkey);
    engine = RenderEngine::get_instance();
}
void *RenderCommandDispatcher::update(RenderResource *resource, u32 offset,
                                      u32 size, void *data) {
    if (resource->type == RenderResourceType::UNINITIALIZE) return NULL;
    RenderCommand cmd;
    cmd.sort_key = get_sort_key();
    cmd.type = RenderCommandType::UPDATE;
    if (data != nullptr) {
        cmd.data = data;
    } else {
        cmd.data = RenderEngine::get_instance()->get_mem_pool()->alloc(size);
    }
    cmd.resource = resource;
    cmd.buffer.size = size;
    cmd.buffer.offset = offset;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    return cmd.data;
}

void *RenderCommandDispatcher::update(RenderResource *resource, u16 x_off,
                                      u16 y_off, u16 w, u16 h, void *data) {
    if (resource->type != RenderResourceType::TEXTURE) return NULL;
    RenderCommand cmd;
    cmd.sort_key = get_sort_key();

    cmd.type = RenderCommandType::UPDATE;
    if (data != nullptr) {
        cmd.data = data;
    } else {
        cmd.data = RenderEngine::get_instance()->get_mem_pool()->alloc(w * h);
    }
    cmd.resource = resource;
    cmd.texture.x_off = x_off;
    cmd.texture.y_off = y_off;
    cmd.texture.w = w;
    cmd.texture.w = h;

    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    return cmd.data;
}

void RenderCommandDispatcher::use(RenderResource *resource) {
    if (resource->type == RenderResourceType::UNINITIALIZE) return;
    RenderCommand cmd;
    cmd.sort_key = get_sort_key();

    cmd.type = RenderCommandType::USE;
    cmd.resource = resource;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}

void RenderCommandDispatcher::use_texture(RenderResource *resource, u32 texture_unit){
    if (resource->type == RenderResourceType::UNINITIALIZE) return;
    if (resource->type != RenderResourceType::TEXTURE){
        SPDLOG_ERROR("The render resource is not a Texture.");
        return;
    }
    RenderCommand cmd;
    cmd.sort_key = get_sort_key();

    cmd.type = RenderCommandType::USE;
    cmd.resource = resource;
    cmd.texture_unit = texture_unit;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}
void RenderCommandDispatcher::use_vertex(RenderResource *resource, VertexDescription *desc){
    if (resource->type == RenderResourceType::UNINITIALIZE) return;
    if (resource->type != RenderResourceType::VERTEX){
        SPDLOG_ERROR("The render resource is not a Vertex.");
        return;
    }
    RenderCommand cmd;
    cmd.sort_key = get_sort_key();

    cmd.type = RenderCommandType::USE;
    cmd.resource = resource;
    cmd.desc = desc;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}
void RenderCommandDispatcher::render(RenderResource *shader,
                                     RenderPrimitiveType prim_type,
                                     u32 instance_cnt, bool indexed) {
    RenderCommand cmd;
    cmd.sort_key = get_sort_key();

    if (shader->type != RenderResourceType::SHADER) {
        SPDLOG_ERROR("The render resource is not a shader.");
        return;
    }
    cmd.type = RenderCommandType::RENDER;
    cmd.resource = shader;
    cmd.render.instance_cnt = instance_cnt;
    cmd.render.prim_type = prim_type;
    cmd.render.indexed = indexed;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}
void RenderCommandDispatcher::end() {
    if (!sort_keys.empty()) sort_keys.pop();
    this->engine = nullptr;
}
}  // namespace Seed