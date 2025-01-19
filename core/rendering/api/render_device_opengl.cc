#include "render_device_opengl.h"
#include <glad/glad.h>

namespace Seed {


RenderDeviceOpenGL::RenderDeviceOpenGL() {
    glGenVertexArrays(1, &global_vao);
    glBindVertexArray(global_vao);
}
void RenderDeviceOpenGL::alloc_texture(RenderResource *rc, u32 w, u32 h,
                                       void *data) {
    glGenTextures(1, &rc->handle);
    glBindTexture(GL_TEXTURE_2D, rc->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void RenderDeviceOpenGL::alloc_vertex(RenderResource *rc, u32 stride,
                                      u32 element_cnt, void *data) {
    rc->element_cnt = element_cnt;
    glGenBuffers(1, &rc->handle);
    glBindBuffer(GL_ARRAY_BUFFER, rc->handle);
    glBufferData(GL_ARRAY_BUFFER, element_cnt * stride, data, GL_STATIC_DRAW);
}

void RenderDeviceOpenGL::alloc_vertex_desc(
    RenderResource *rc, std::vector<VertexAttribute> &attrs) {
    rc->handle = vertex_attrs.size();
    vertex_attrs.push_back(attrs);
}

void RenderDeviceOpenGL::alloc_indices(RenderResource *rc,
                                       std::vector<u32> &indices) {
    glGenBuffers(1, &rc->handle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc->handle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(u32), indices.data(),
                 GL_STATIC_DRAW);
}

void RenderDeviceOpenGL::alloc_shader(RenderResource *rc,
                                      const char *vertex_code,
                                      const char *fragment_code) {
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
    rc->handle = id;
}
void RenderDeviceOpenGL::alloc_constant(RenderResource *rc, u32 size,
                                        void *data) {
    glGenBuffers(1, &rc->handle);
    glBindBuffer(GL_UNIFORM_BUFFER, rc->handle);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void RenderDeviceOpenGL::dealloc(RenderResource *r) {}

void RenderDeviceOpenGL::handle_update(RenderCommand &cmd) {
    RenderResource *rc = cmd.resource;
    switch (rc->type) {
        case RenderResourceType::VERTEX:
            u32 vbo;
            glBindBuffer(GL_ARRAY_BUFFER, rc->handle);
            if (rc->element_cnt * rc->stride < cmd.buffer.size) {
                glBufferData(GL_ARRAY_BUFFER, cmd.buffer.size, cmd.data,
                             GL_DYNAMIC_DRAW);
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, cmd.buffer.offset,
                                cmd.buffer.size, cmd.data);
            }

            break;
        case RenderResourceType::TEXTURE:
            glBindTexture(GL_TEXTURE_2D, rc->handle);
            glTexSubImage2D(GL_TEXTURE_2D, 0, cmd.texture.x_off,
                            cmd.texture.y_off, cmd.texture.w, cmd.texture.h,
                            GL_RGBA, GL_UNSIGNED_BYTE, cmd.data);
            glBindTexture(GL_TEXTURE_2D, 0);

            break;
        case RenderResourceType::CONSTANT:
            glBindBuffer(GL_UNIFORM_BUFFER, rc->handle);
            glBufferSubData(GL_UNIFORM_BUFFER, cmd.buffer.offset,
                            cmd.buffer.size, cmd.data);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            break;
        case RenderResourceType::INDEX:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc->handle);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, cmd.buffer.offset,
                            cmd.buffer.size, cmd.data);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            break;
        default:
            break;
    }
}
void RenderDeviceOpenGL::use_vertex_desc(u32 handle) {
    std::vector<VertexAttribute> &attrs = vertex_attrs[handle];
    int cnt = 0;
    for (VertexAttribute &attr : attrs) {
        u32 type;
        u32 size;
        switch (attr.type) {
            case VertexAttributeType::UNSIGNED:
                type = GL_UNSIGNED_INT;
                size = sizeof(u32);
                break;
            case VertexAttributeType::INT:
                type = GL_INT;
                size = sizeof(i32);
                break;
            case VertexAttributeType::FLOAT:
            default:
                type = GL_FLOAT;
                size = sizeof(f32);
                break;
        }
        glEnableVertexAttribArray(attr.layout_num);
        glVertexAttribPointer(attr.layout_num, attr.size, type,
                              attr.should_normalized, attr.stride,
                              (void *)(size_t)cnt);
        glVertexAttribDivisor(attr.layout_num, attr.is_instance ? 1 : 0);
        cnt += attr.size * size;
    }
    glEnableVertexAttribArray(0);
}
void RenderDeviceOpenGL::handle_use(RenderCommand &cmd) {
    RenderResource *rc = cmd.resource;

    switch (rc->type) {
        case RenderResourceType::VERTEX:
            glBindBuffer(GL_ARRAY_BUFFER, rc->handle);
            break;
        case RenderResourceType::INDEX:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc->handle);
            element_cnt = rc->element_cnt;

            break;
        case RenderResourceType::TEXTURE:
            glBindTexture(GL_TEXTURE_2D, rc->handle);
            break;

        case RenderResourceType::VERTEX_DESC:
            use_vertex_desc(rc->handle);
            break;
        default:
            break;
    }
}

void RenderDeviceOpenGL::handle_render(RenderCommand &cmd) {
    RenderResource *rc = cmd.resource;
    glUseProgram(rc->handle);
    glDrawElementsInstanced(GL_TRIANGLES, element_cnt, GL_UNSIGNED_INT, 0, cmd.render.instance_cnt);
}

void RenderDeviceOpenGL::process() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    while (!cmd_queue.empty()) {
        RenderCommand &cmd = cmd_queue.front();
        cmd_queue.pop();

        switch (cmd.type) {
            case RenderCommandType::UPDATE:
                handle_update(cmd);
                break;
            case RenderCommandType::USE:
                handle_use(cmd);
                break;
            case RenderCommandType::RENDER:
                handle_render(cmd);
                break;
            default:
                break;
        }
    }

}
}  // namespace Seed