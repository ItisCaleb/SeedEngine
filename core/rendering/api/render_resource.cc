#include "render_resource.h"
#include <glad/glad.h>
#include <fmt/core.h>

namespace Seed {

void RenderResource::alloc_texture(u32 w, u32 h, void* data) {
    this->type = RenderResourceType::TEXTURE;
    glGenTextures(1, &this->handle);
    glBindTexture(GL_TEXTURE_2D, this->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void RenderResource::alloc_vertex(u32 stride, std::vector<u32> &lens,
                                  u32 element_cnt, void *data) {
    this->type = RenderResourceType::VERTEX;
    glGenVertexArrays(1, &this->handle);
    glBindVertexArray(this->handle);
    this->element_cnt = element_cnt;
    u32 cnt = 0;
    u32 vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, element_cnt * stride, data, GL_DYNAMIC_DRAW);
    for (int i = 0; i < lens.size(); i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, lens[i], GL_FLOAT, GL_FALSE, stride,
                              (void *)(size_t)cnt);
        cnt += lens[i] * sizeof(f32);
    }
    glEnableVertexAttribArray(0);
}
void RenderResource::alloc_shader(const char *vertex_code,
                                  const char *fragment_code) {
    this->type = RenderResourceType::SHADER;

    u32 vertex, fragment;
    int success;
    char info[512];
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_code, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, info);
        throw std::exception(info);
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_code, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, info);
        throw std::exception(info);
    }
    u32 id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, info);
        throw std::exception(info);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    this->handle = id;
}
void RenderResource::alloc_constant(u32 size, void *data) {
    this->type = RenderResourceType::CONSTANT;

    glGenBuffers(1, &this->handle);
    glBindBuffer(GL_UNIFORM_BUFFER, this->handle);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void RenderResource::dealloc() {}

void RenderResource::register_resource(const std::string &name, RenderResource rc){
    constants[name] = rc;
    if(rc.type == RenderResourceType::CONSTANT){
        for(auto &e:shaders){
            u32 idx = glGetUniformBlockIndex(e.second.handle, name.c_str());
            glUniformBlockBinding(e.second.handle, idx, constant_cnt);
        }
        glBindBufferBase(GL_UNIFORM_BUFFER, constant_cnt, rc.handle); 
        constant_cnt++;
    }
    if(rc.type == RenderResourceType::SHADER){
        shaders[name] = rc;
    }
}

}  // namespace Seed