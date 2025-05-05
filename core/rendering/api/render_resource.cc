#include "render_resource.h"
#include <glad/glad.h>
#include <fmt/core.h>
#include "render_engine.h"

namespace Seed {

void RenderResource::alloc_texture(u32 w, u32 h, const void *data) {
    this->type = RenderResourceType::TEXTURE;
    RenderEngine::get_instance()->get_device()->alloc_texture(this, w, h, data);
}
void RenderResource::alloc_vertex(u32 stride, u32 vertex_cnt,const void *data) {
    this->type = RenderResourceType::VERTEX;
    this->stride = stride;
    this->vertex_cnt = vertex_cnt;
    RenderEngine::get_instance()->get_device()->alloc_vertex(this, stride,
                                                             vertex_cnt, data);
}
void RenderResource::alloc_shader(const std::string &vertex_code,
    const std::string &fragment_code,
     const std::string &geometry_code, const std::string &tess_ctrl_code, const std::string &tess_eval_code) {
    this->type = RenderResourceType::SHADER;
    RenderEngine::get_instance()->get_device()->alloc_shader(
        this, vertex_code, fragment_code, geometry_code, tess_ctrl_code, tess_eval_code);
}
void RenderResource::alloc_index(const std::vector<u32> &indices) {
    this->type = RenderResourceType::INDEX;
    this->element_cnt = indices.size();
    RenderEngine::get_instance()->get_device()->alloc_indices(this, indices);
}

void RenderResource::alloc_constant(const std::string &name, u32 size,
                                    void *data) {
    this->type = RenderResourceType::CONSTANT;
    RenderEngine::get_instance()->get_device()->alloc_constant(this, name, size,
                                                               data);
}
void RenderResource::dealloc() {
    if (this->type == RenderResourceType::UNINITIALIZE) {
        return;
    }
    RenderEngine::get_instance()->get_device()->dealloc(this);
    this->handle = 0;
    this->type = RenderResourceType::UNINITIALIZE;
}

bool RenderResource::inited() {
    return this->type != RenderResourceType::UNINITIALIZE;
}

}  // namespace Seed