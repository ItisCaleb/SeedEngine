#include "render_resource.h"
#include <glad/glad.h>
#include <fmt/core.h>
#include "render_engine.h"

namespace Seed {

void RenderResource::alloc_texture(u32 w, u32 h, void *data) {
    this->type = RenderResourceType::TEXTURE;
    RenderEngine::get_instance()->get_device()->alloc_texture(this, w, h, data);
}
void RenderResource::alloc_vertex(u32 stride,
                                  u32 element_cnt, void *data) {
    this->type = RenderResourceType::VERTEX;
    this->stride = stride;
    RenderEngine::get_instance()->get_device()->alloc_vertex(
        this, stride, element_cnt, data);
}
void RenderResource::alloc_shader(const char *vertex_code,
                                  const char *fragment_code) {
    this->type = RenderResourceType::SHADER;
    RenderEngine::get_instance()->get_device()->alloc_shader(this, vertex_code,
                                                             fragment_code);
}
void RenderResource::alloc_index(std::vector<u32> &indices) {
    this->type = RenderResourceType::INDEX;
    this->element_cnt = indices.size();
    RenderEngine::get_instance()->get_device()->alloc_indices(this, indices);
}
void RenderResource::alloc_vertex_desc(std::vector<VertexAttribute> &attrs) {
    this->type = RenderResourceType::VERTEX_DESC;
    RenderEngine::get_instance()->get_device()->alloc_vertex_desc(this, attrs);
}

void RenderResource::alloc_constant(u32 size, void *data) {
    this->type = RenderResourceType::CONSTANT;
    RenderEngine::get_instance()->get_device()->alloc_constant(this, size,
                                                               data);
}
void RenderResource::dealloc() {}

void RenderResource::register_resource(const std::string &name,
                                       RenderResource rc) {
    constants[name] = rc;
    if (rc.type == RenderResourceType::CONSTANT) {
        for (auto &e : shaders) {
            u32 idx = glGetUniformBlockIndex(e.second.handle, name.c_str());
            glUniformBlockBinding(e.second.handle, idx, constant_cnt);
        }
        glBindBufferBase(GL_UNIFORM_BUFFER, constant_cnt, rc.handle);
        constant_cnt++;
    }
    if (rc.type == RenderResourceType::SHADER) {
        shaders[name] = rc;
    }
}

}  // namespace Seed