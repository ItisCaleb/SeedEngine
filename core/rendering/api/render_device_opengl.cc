#include "render_device_opengl.h"
#include "render_engine.h"
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
                                      u32 vertex_cnt, const void *data) {
    GLuint handle;
    glGenBuffers(1, &handle);
    glBindBuffer(GL_ARRAY_BUFFER, handle);
    glBufferData(GL_ARRAY_BUFFER, vertex_cnt * stride, data, GL_STATIC_DRAW);
    rc->handle = this->buffers.insert(
        HardwareBufferGL{.handle = handle, .size = vertex_cnt * stride});
}

void RenderDeviceOpenGL::alloc_indices(RenderResource *rc, IndexType type,
                                       u32 element_cnt, void *data) {
    GLuint handle;
    u32 type_size = 0;
    switch (type) {
        case IndexType::UNSIGNED_BYTE:
            type_size = 1;
            break;
        case IndexType::UNSIGNED_SHORT:
            type_size = 2;
            break;
        case IndexType::UNSIGNED_INT:
            type_size = 4;
            break;
        default:
            break;
    }
    glGenBuffers(1, &handle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_cnt * type_size, data,
                 GL_STATIC_DRAW);
    rc->handle = this->buffers.insert(
        HardwareBufferGL{.handle = handle, .size = element_cnt * type_size});
}

void RenderDeviceOpenGL::alloc_constant(RenderResource *rc,
                                        const std::string &name, u32 size,
                                        void *data) {
    GLuint handle;
    glGenBuffers(1, &handle);
    glBindBuffer(GL_UNIFORM_BUFFER, handle);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    for (auto &e : shaders) {
        u32 idx = glGetUniformBlockIndex(e.second.handle, name.c_str());
        if (idx != GL_INVALID_INDEX) {
            glUniformBlockBinding(e.second.handle, idx, constant_cnt);
        }
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, constant_cnt, handle);
    constant_cnt++;
    rc->handle =
        this->buffers.insert(HardwareBufferGL{.handle = handle, .size = size});
}

void RenderDeviceOpenGL::alloc_shader(RenderResource *rc,
                                      const std::string &vertex_code,
                                      const std::string &fragment_code,
                                      const std::string &geometry_code,
                                      const std::string &tess_ctrl_code,
                                      const std::string &tess_eval_code) {
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
    } else if (tess_ctrl_code.size() == 0 && tess_eval_code.size() > 0 ||
               tess_ctrl_code.size() > 0 && tess_eval_code.size() == 0) {
        throw std::runtime_error(
            "TCS and TES need to be provide at same time.");
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

void RenderDeviceOpenGL::dealloc(RenderResource *r) {
    switch (r->type) {
        case RenderResourceType::VERTEX:
        case RenderResourceType::INDEX: {
            HardwareBufferGL &hb = this->buffers[r->handle];
            glDeleteBuffers(1, &hb.handle);
            this->buffers.erase(r->handle);
        } break;
        case RenderResourceType::TEXTURE: {
            HardwareBufferGL &hb = this->buffers[r->handle];
            glDeleteTextures(1, &hb.handle);
            this->buffers.erase(r->handle);
        } break;
        case RenderResourceType::SHADER:
            glDeleteProgram(r->handle);
            break;
        default:
            break;
    }
}

void RenderDeviceOpenGL::handle_update(RenderCommand &cmd) {
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);
    RenderResource *buffer = update_data->dst_buffer;
    HardwareBufferGL &hb = this->buffers[buffer->handle];
    switch (buffer->type) {
        case RenderResourceType::VERTEX:
            u32 vbo;
            glBindBuffer(GL_ARRAY_BUFFER, hb.handle);
            if (hb.size < update_data->buffer.size) {
                glBufferData(GL_ARRAY_BUFFER, update_data->buffer.size,
                             update_data->data, GL_DYNAMIC_DRAW);
                hb.size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, update_data->buffer.offset,
                                update_data->buffer.size, update_data->data);
            }
            break;
        case RenderResourceType::TEXTURE:
            glBindTexture(GL_TEXTURE_2D, buffer->handle);
            glTexSubImage2D(GL_TEXTURE_2D, 0, update_data->texture.x_off,
                            update_data->texture.y_off, update_data->texture.w,
                            update_data->texture.h, GL_RGBA, GL_UNSIGNED_BYTE,
                            update_data->data);
            glBindTexture(GL_TEXTURE_2D, 0);

            break;
        case RenderResourceType::CONSTANT:
            glBindBuffer(GL_UNIFORM_BUFFER, hb.handle);
            if (hb.size < update_data->buffer.size) {
                glBufferData(GL_UNIFORM_BUFFER, update_data->buffer.size,
                             update_data->data, GL_DYNAMIC_DRAW);
                hb.size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_UNIFORM_BUFFER, update_data->buffer.offset,
                                update_data->buffer.size, update_data->data);
            }
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            break;
        case RenderResourceType::INDEX:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hb.handle);
            if (hb.size < update_data->buffer.size) {
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, update_data->buffer.size,
                             update_data->data, GL_DYNAMIC_DRAW);
                hb.size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                                update_data->buffer.offset,
                                update_data->buffer.size, update_data->data);
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            break;
        default:
            break;
    }
}
void RenderDeviceOpenGL::use_vertex_desc(VertexDescription *desc) {
    if (!desc) {
        SPDLOG_ERROR("VertexDescription is null");
        return;
    }
    int cnt = 0;
    for (VertexAttribute &attr : desc->get_attrs()) {
        u32 type;
        u32 size;
        switch (attr.type) {
            case VertexAttributeType::UNSIGNED_BYTE:
                type = GL_UNSIGNED_BYTE;
                size = sizeof(u8);
                break;
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
                              attr.should_normalized, desc->get_stride(),
                              (void *)(size_t)cnt);
        glVertexAttribDivisor(attr.layout_num, attr.instance_step);
        cnt += attr.size * size;
    }
    glEnableVertexAttribArray(0);
}

void RenderDeviceOpenGL::bind_buffer(RenderResource *rc) {
    HardwareBufferGL &hb = this->buffers[rc->handle];
    switch (rc->type) {
        case RenderResourceType::VERTEX:
            glBindBuffer(GL_ARRAY_BUFFER, hb.handle);
            break;
        case RenderResourceType::INDEX:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hb.handle);
        default:
            break;
    }
}

void RenderDeviceOpenGL::setup_rasterizer(RenderRasterizerState &state) {
    switch (state.cull_mode) {
        case RenderRasterizerState::Cullmode::FRONT:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case RenderRasterizerState::Cullmode::BACK:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case RenderRasterizerState::Cullmode::BOTH:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case RenderRasterizerState::Cullmode::NONE:
        default:
            glDisable(GL_CULL_FACE);
            break;
    }
    glPatchParameteri(GL_PATCH_VERTICES, state.patch_control_points);
}

void RenderDeviceOpenGL::setup_depth_stencil(RenderDepthStencilState &state) {
    if (state.depth_on)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (state.stencil_on)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
}

inline static u32 get_blend_func(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::ZERO:
            return GL_ZERO;
        case BlendFactor::ONE:
            return GL_ONE;
        case BlendFactor::SRC_COLOR:
            return GL_SRC_COLOR;
        case BlendFactor::ONE_MINUS_SRC_COLOR:
            return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DST_COLOR:
            return GL_DST_COLOR;
        case BlendFactor::ONE_MINUS_DST_COLOR:
            return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SRC_ALPHA:
            return GL_SRC_ALPHA;
        case BlendFactor::ONE_MINUS_SRC_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DST_ALPHA:
            return GL_DST_ALPHA;
        case BlendFactor::ONE_MINUS_DST_ALPHA:
            return GL_ONE_MINUS_DST_ALPHA;
        case BlendFactor::CONSTANT_COLOR:
            return GL_CONSTANT_COLOR;
        case BlendFactor::ONE_MINUS_CONSTANT_COLOR:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::CONSTANT_ALPHA:
            return GL_CONSTANT_COLOR;
        case BlendFactor::ONE_MINUS_CONSTANT_ALPHA:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case BlendFactor::SRC_ALPHA_SATURATE:
            return GL_SRC_ALPHA_SATURATE;
        case BlendFactor::SRC1_COLOR:
            return GL_SRC1_COLOR;
        case BlendFactor::ONE_MINUS_SRC1_COLOR:
            return GL_ONE_MINUS_SRC1_COLOR;
        case BlendFactor::SRC1_ALPHA:
            return GL_SRC1_ALPHA;
        case BlendFactor::ONE_MINUS_SRC1_ALPHA:
            return GL_ONE_MINUS_SRC1_ALPHA;
    }
}
void RenderDeviceOpenGL::setup_blend(RenderBlendState &state) {
    if (state.blend_on){
        glEnable(GL_BLEND);
        glBlendFuncSeparate(get_blend_func(state.func.src_rgb),
        get_blend_func(state.func.dst_rgb),
        get_blend_func(state.func.src_alpha),
        get_blend_func(state.func.dst_alpha));
    }
    else
        glDisable(GL_BLEND);

}

void RenderDeviceOpenGL::handle_state(RenderCommand &cmd) {
    RenderStateData *state_data = static_cast<RenderStateData *>(cmd.data);
    u64 flag = state_data->flag;

    if (flag & RenderStateFlag::CLEAR) {
        GLuint clear_flag = 0;
        if (state_data->clear_flag & StateClearFlag::CLEAR_COLOR) {
            clear_flag |= GL_COLOR_BUFFER_BIT;
        }
        if (state_data->clear_flag & StateClearFlag::CLEAR_DEPTH) {
            clear_flag |= GL_DEPTH_BUFFER_BIT;
        }
        if (state_data->clear_flag & StateClearFlag::CLEAR_STENCIL) {
            clear_flag |= GL_STENCIL_BUFFER_BIT;
        }
        glClear(clear_flag);
    }
    if (flag & RenderStateFlag::VIEWPORT) {
        ViewportState &vp = state_data->viewport;
        glViewport(vp.x, vp.y, vp.w, vp.h);
    }
    if (flag & RenderStateFlag::SCISSOR) {
        ScissorRect &rect = state_data->rect;
        glEnable(GL_SCISSOR_TEST);
        glScissor(rect.x, rect.y, rect.w, rect.h);
    }
}

void RenderDeviceOpenGL::handle_render(RenderCommand &cmd) {
    RenderDrawData *draw_data = static_cast<RenderDrawData *>(cmd.data);
    if (draw_data->set_viewport) {
        ViewportState &vp = draw_data->viewport;
        glViewport(vp.x, vp.y, vp.w, vp.h);
    }
    if (draw_data->set_scissor) {
        ScissorRect &rect = draw_data->scissor;
        glEnable(GL_SCISSOR_TEST);
        glScissor(rect.x, rect.y, rect.w, rect.h);
    }
    if (this->pipeline != draw_data->pipeline) {
        this->pipeline = draw_data->pipeline;
        u32 shader_handle = this->pipeline->shader->get_rc()->handle;
        glUseProgram(shader_handle);
        for (int i = 0; i < 8; i++) {
            std::string name = fmt::format("u_texture[{}]", i);
            u32 loc = glGetUniformLocation(shader_handle, name.c_str());
            if (loc == -1) break;
            glUniform1i(loc, i);
        }
        setup_rasterizer(this->pipeline->rst_state);
        setup_depth_stencil(this->pipeline->depth_state);
        setup_blend(this->pipeline->blend_state);
    }
    /* select primitive to draw */
    u32 prim_type;
    switch (pipeline->type) {
        case RenderPrimitiveType::POINTS:
            prim_type = GL_POINTS;
            break;
        case RenderPrimitiveType::LINES:
            prim_type = GL_LINES;
            break;
        case RenderPrimitiveType::PATCHES:
            prim_type = GL_PATCHES;
            break;
        case RenderPrimitiveType::TRIANGLES:
        default:
            prim_type = GL_TRIANGLES;
            break;
    }
    VertexData *vertices = draw_data->vertices;
    bind_buffer(vertices->get_vertices());
    use_vertex_desc(pipeline->desc);
    if (vertices->use_index()) {
        bind_buffer(vertices->get_indices());
    }
    if (draw_data->instance_cnt > 0 && pipeline->instance_desc) {
        bind_buffer(draw_data->instance);
        use_vertex_desc(pipeline->instance_desc);
    }

    Ref<Material> mat = draw_data->mat;
    if (mat.is_null()) {
        mat = RenderEngine::get_instance()->get_default_material();
    }
    if (mat->get_id() != this->last_material) {
        this->last_material = mat->get_id();
        for (u32 i = 0; i < Material::MAX; i++) {
            Ref<Texture> tex =
                mat->get_texture_map(static_cast<Material::TextureMapType>(i));
            if (tex.is_null()) {
                tex = RenderEngine::get_instance()->get_default_texture();
            }
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, tex->get_render_resource()->handle);
        }
    }
    u32 index_type = 0;
    switch (vertices->get_index_type()) {
        case IndexType::UNSIGNED_BYTE:
            index_type = GL_UNSIGNED_BYTE;
            break;
        case IndexType::UNSIGNED_SHORT:
            index_type = GL_UNSIGNED_SHORT;
            break;
        case IndexType::UNSIGNED_INT:
            index_type = GL_UNSIGNED_INT;
            break;
    }

    if (vertices->use_index()) {
        if (draw_data->instance_cnt > 0) {
            glDrawElementsInstanced(prim_type, draw_data->index_cnt, index_type,
                                    (void *)(u64)draw_data->index_offset,
                                    draw_data->instance_cnt);
        } else {
            glDrawElements(prim_type, draw_data->index_cnt, index_type,
                           (void *)(u64)draw_data->index_offset);
        }
    } else {
        if (draw_data->instance_cnt > 0) {
            glDrawArraysInstanced(prim_type, 0, vertices->get_vertices_cnt(),
                                  draw_data->instance_cnt);
        } else {
            glDrawArrays(prim_type, 0, vertices->get_vertices_cnt());
        }
    }
}

void RenderDeviceOpenGL::process() {
    std::stable_sort(std::begin(cmd_queue), std::end(cmd_queue),
                     RenderCommand::cmp);

    while (!cmd_queue.empty()) {
        RenderCommand &cmd = cmd_queue.front();
        cmd_queue.pop_front();
        switch (cmd.type) {
            case RenderCommandType::UPDATE:
                handle_update(cmd);
                static_cast<RenderUpdateData *>(cmd.data)->~RenderUpdateData();
                break;
            case RenderCommandType::STATE:
                handle_state(cmd);
                static_cast<RenderStateData *>(cmd.data)->~RenderStateData();
                break;
            case RenderCommandType::RENDER:
                handle_render(cmd);
                static_cast<RenderDrawData *>(cmd.data)->~RenderDrawData();
                break;
            default:
                break;
        }
    }
}
}  // namespace Seed