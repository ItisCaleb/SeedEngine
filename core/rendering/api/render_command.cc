#include "render_command.h"
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {
void RenderCommandDispatcher::begin() { engine = RenderEngine::get_instance(); }
void *RenderCommandDispatcher::update(RenderResource *resource, u32 offset,
                                      u32 size, void *data) {
    if (resource->type == RenderResourceType::UNINITIALIZE ||
        resource->type == RenderResourceType::TEXTURE)
        return NULL;
    RenderCommand cmd;
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
    cmd.type = RenderCommandType::UPDATE;
    if (data != nullptr) {
        cmd.data = data;
    } else {
        cmd.data = RenderEngine::get_instance()->get_mem_pool()->alloc(w*h);
    }
    cmd.resource = resource;
    cmd.texture.x_off = x_off;
    cmd.texture.y_off = y_off;
    cmd.texture.w = w;
    cmd.texture.w = h;

    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
    return cmd.data;
}

void RenderCommandDispatcher::use(RenderResource *resource, u32 texutre_unit) {
    if (resource->type == RenderResourceType::UNINITIALIZE) return;
    RenderCommand cmd;
    cmd.type = RenderCommandType::USE;
    cmd.resource = resource;
    cmd.texture_unit = texutre_unit;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}
void RenderCommandDispatcher::render(RenderResource *shader, u32 instance_cnt) {
    RenderCommand cmd;
    if (shader->type != RenderResourceType::SHADER) {
        spdlog::error("The render resource is not a shader.");
        return;
    }
    cmd.type = RenderCommandType::RENDER;
    cmd.resource = shader;
    cmd.render.instance_cnt = instance_cnt;
    RenderEngine::get_instance()->get_device()->push_cmd(cmd);
}
void RenderCommandDispatcher::end() { this->engine = nullptr; }
}  // namespace Seed