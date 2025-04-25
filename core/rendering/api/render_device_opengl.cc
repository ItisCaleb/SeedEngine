#include "render_device_opengl.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace Seed {

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id,
                            GLenum severity, GLsizei length,
                            const char *message, const void *userParam) {
    std::string source_str;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            break;
        default:
            return;
    }
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            source_str = "Source: API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_str = "Source: Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_str = "Source: Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_str = "Source: Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_str = "Source: Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            source_str = "Source: Other";
            break;
    }
    spdlog::error("{}: {} ({})", source_str, message, userParam);
    return;
}

RenderDeviceOpenGL::RenderDeviceOpenGL() {
    glGenVertexArrays(1, &global_vao);
    glBindVertexArray(global_vao);
    int flags;
    std::string version = (char *)glGetString(GL_VERSION);
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if ((flags & GL_CONTEXT_FLAG_DEBUG_BIT) &&
        version.rfind("4.3") != std::string::npos) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                              nullptr, GL_TRUE);
    }
}
void RenderDeviceOpenGL::alloc_texture(RenderResource *rc, u32 w, u32 h,
                                       const void *data) {
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
                                      u32 vertex_cnt, void *data) {
    glGenBuffers(1, &rc->handle);
    glBindBuffer(GL_ARRAY_BUFFER, rc->handle);
    glBufferData(GL_ARRAY_BUFFER, vertex_cnt * stride, data, GL_STATIC_DRAW);
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(u32),
                 indices.data(), GL_STATIC_DRAW);
}

void RenderDeviceOpenGL::alloc_shader(RenderResource *rc, const std::string &vertex_code,
    const std::string &fragment_code,
    const std::string &geometry_code,const std::string &tess_ctrl_code, const std::string &tess_eval_code) {
    u32 vertex, fragment, geometry, tess_ctrl, tess_eval;
    int success;
    char info[512];
    u32 program = glCreateProgram();
    const char *vertex_c = vertex_code.c_str();
    const char *frag_c = fragment_code.c_str();
    const char *geo_c = geometry_code.c_str();
    const char *tess_ctrl_c = tess_ctrl_code.c_str();
    const char *tess_eval_c = tess_eval_code.c_str();


    /* vertex shader */
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_c, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, info);
        throw std::runtime_error(info);
    }
    glAttachShader(program, vertex);

    /* fragment shader */
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &frag_c, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, info);
        throw std::runtime_error(info);
    }
    glAttachShader(program, fragment);

    /* optional geometry shader */
    if (geometry_code.size() > 0) {
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &geo_c, NULL);
        glCompileShader(geometry);
        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(geometry, 512, NULL, info);
            throw std::runtime_error(info);
        }
        glAttachShader(program, geometry);
    }

    /* optional tesselation shader */
    if (tess_ctrl_code.size() > 0 && tess_eval_code.size()) {
        tess_ctrl = glCreateShader(GL_TESS_CONTROL_SHADER);
        tess_eval = glCreateShader(GL_TESS_EVALUATION_SHADER);

        glShaderSource(tess_ctrl, 1, &tess_ctrl_c, NULL);
        glShaderSource(tess_eval, 1, &tess_eval_c, NULL);

        glCompileShader(tess_ctrl);
        glCompileShader(tess_eval);
        glGetShaderiv(tess_ctrl, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(tess_ctrl, 512, NULL, info);
            throw std::runtime_error(info);
        }
        glGetShaderiv(tess_eval, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(tess_ctrl, 512, NULL, info);
            throw std::runtime_error(info);
        }
        glAttachShader(program, tess_ctrl);
        glAttachShader(program, tess_eval);
    }else if(tess_ctrl_code.size() == 0 && tess_eval_code.size() > 0 ||
    tess_ctrl_code.size() > 0 && tess_eval_code.size() == 0){
        throw std::runtime_error("TCS and TES need to be provide at same time.");
    }

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, info);
        throw std::runtime_error(info);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    rc->handle = program;
    shaders[rc->handle] = *rc;
}
void RenderDeviceOpenGL::alloc_constant(RenderResource *rc, const std::string &name, u32 size,
                                        void *data) {
    glGenBuffers(1, &rc->handle);
    glBindBuffer(GL_UNIFORM_BUFFER, rc->handle);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    constants[name] = *rc;
    for (auto &e : shaders) {
        u32 idx = glGetUniformBlockIndex(e.second.handle, name.c_str());
        if(idx != GL_INVALID_INDEX){
            glUniformBlockBinding(e.second.handle, idx, constant_cnt);
        }
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, constant_cnt, rc->handle);
    constant_cnt++;
}
void RenderDeviceOpenGL::dealloc(RenderResource *r) {
    switch (r->type) {
        case RenderResourceType::VERTEX:
        case RenderResourceType::INDEX:
            glDeleteBuffers(1, &r->handle);
            break;
        case RenderResourceType::TEXTURE:
            glDeleteTextures(1, &r->handle);
            break;
        case RenderResourceType::SHADER:
            glDeleteProgram(r->handle);
            break;
        default:
            break;
    }
}

void RenderDeviceOpenGL::handle_update(RenderCommand &cmd) {
    RenderResource *rc = cmd.resource;
    switch (rc->type) {
        case RenderResourceType::VERTEX:
            u32 vbo;
            glBindBuffer(GL_ARRAY_BUFFER, rc->handle);
            if (rc->vertex_cnt * rc->stride < cmd.buffer.size) {
                glBufferData(GL_ARRAY_BUFFER, cmd.buffer.size, cmd.data,
                             GL_DYNAMIC_DRAW);
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, cmd.buffer.offset,
                                cmd.buffer.size, cmd.data);
            }
            rc->vertex_cnt = cmd.buffer.size / rc->stride;
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
            rc->element_cnt = cmd.buffer.size / sizeof(u32);
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
            vertex_cnt = rc->vertex_cnt;
            break;
        case RenderResourceType::INDEX:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc->handle);
            element_cnt = rc->element_cnt;
            break;
        case RenderResourceType::TEXTURE:
            glActiveTexture(GL_TEXTURE0 + cmd.texture_unit);
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
    if (current_program != rc->handle) {
        glUseProgram(rc->handle);
        current_program = rc->handle;
        /* bind texture unit in fragment shader */
        for (int i = 0; i < 8; i++) {
            std::string name = fmt::format("u_texture[{}]", i);
            u32 loc = glGetUniformLocation(rc->handle, name.c_str());
            if (loc == -1) break;
            glUniform1i(loc, i);
        }
    }
    /* select primitive to draw */
    u32 prim_type;
    switch (cmd.render.prim_type) {
        case RenderPrimitiveType::POINTS:
            prim_type = GL_POINTS;
            break;
        case RenderPrimitiveType::LINES:
            prim_type = GL_LINES;
            break;
        case RenderPrimitiveType::TRIANGLES:
        default:
            prim_type = GL_TRIANGLES;
            break;
    }
    if (cmd.render.indexed) {
        if (cmd.render.instance_cnt > 0) {
            glDrawElementsInstanced(prim_type, element_cnt, GL_UNSIGNED_INT, 0,
                                    cmd.render.instance_cnt);
        } else {
            glDrawElements(prim_type, element_cnt, GL_UNSIGNED_INT, 0);
        }
    } else {
        if (cmd.render.instance_cnt > 0) {
            glDrawArraysInstanced(prim_type, 0, vertex_cnt,
                                  cmd.render.instance_cnt);
        } else {
            glDrawArrays(prim_type, 0, vertex_cnt);
        }
    }
}

void RenderDeviceOpenGL::process() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    std::sort(std::begin(cmd_queue), std::end(cmd_queue), RenderCommand::cmp);
    while (!cmd_queue.empty()) {
        RenderCommand &cmd = cmd_queue.front();
        cmd_queue.pop_front();
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