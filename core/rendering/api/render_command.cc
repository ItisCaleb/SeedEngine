#include "render_command.h"
#include "render_engine.h"
#include <fmt/core.h>

namespace Seed {
void RenderCommandDispatcher::begin() { engine = RenderEngine::get_instance(); }
void *RenderCommandDispatcher::update(RenderResource *resource, u32 offset,
                                      u32 size) {
    if(resource->type == RenderResourceType::UNINITIALIZE) return NULL;
    RenderCommand cmd;
    cmd.type = RenderCommandType::UPDATE;
    cmd.data = malloc(size);
    cmd.size = size;
    cmd.offset = offset;
    cmd.resource = resource;
    RenderEngine::get_instance()->push_cmd(cmd);
    return cmd.data;
}

void RenderCommandDispatcher::use(RenderResource *resource) {
    if(resource->type == RenderResourceType::UNINITIALIZE) return;
    RenderCommand cmd;
    cmd.type = RenderCommandType::USE;
    cmd.resource = resource;
    RenderEngine::get_instance()->push_cmd(cmd);
}
void RenderCommandDispatcher::render() {
    RenderCommand cmd;
    cmd.type = RenderCommandType::RENDER;
    RenderEngine::get_instance()->push_cmd(cmd);
}
void RenderCommandDispatcher::end() { this->engine = nullptr; }
}  // namespace Seed