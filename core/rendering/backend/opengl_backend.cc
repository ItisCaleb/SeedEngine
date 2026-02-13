#include "opengl_backend.h"
#include "core/rendering/api/render_engine.h"
#include <spdlog/spdlog.h>
#include "core/macro.h"
#include "opengl_helper.h"
#include <GLFW/glfw3.h>

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

RenderBackendGL::RenderBackendGL() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::error("Can't initialize GLAD. Exiting");
        exit(1);
    }
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
    push_constant.type = RenderResourceType::CONSTANT;
    this->alloc_constant(&push_constant, 0, nullptr);
}
void RenderBackendGL::alloc_texture(RenderResource *rc, TextureType type, u32 w,
                                    u32 h, PixelFormat format,
                                    const SamplerProperty &property,
                                    const void *data) {
    if (rc->type != RenderResourceType::TEXTURE) {
        return;
    }
    HardwareTextureGL texture;
    texture.w = w;
    texture.h = h;
    texture.type = type;
    texture.format = format;
    texture.property = property;
    rc->handle = this->textures.insert(texture);
    std::lock_guard lg(alloc_lock);

    void *tmp = nullptr;
    if (data) {
        size_t size = w * h * get_pixel_format_size(format);
        tmp = malloc(w * h * get_pixel_format_size(format));
        memcpy(tmp, data, size);
    }
    this->alloc_cmds.push(
        AllocCommand{.rc = *rc, .is_alloc = true, .alloc_data = tmp});
}
void RenderBackendGL::alloc_vertex(RenderResource *rc, u32 stride,
                                   u32 vertex_cnt, UpdateFrequence frequence,
                                   const void *data) {
    if (rc->type != RenderResourceType::VERTEX) {
        return;
    }
    HardwareBufferGL buffer;
    buffer.size = stride * vertex_cnt;
    rc->handle = this->vertices.insert(buffer);
    std::lock_guard lg(alloc_lock);
    void *tmp = nullptr;
    if (data) {
        tmp = malloc(buffer.size);
        memcpy(tmp, data, buffer.size);
    }

    this->alloc_cmds.push(
        AllocCommand{.rc = *rc, .is_alloc = true, .alloc_data = tmp});
}

void RenderBackendGL::alloc_indices(RenderResource *rc, IndexType type,
                                    u32 element_cnt, UpdateFrequence frequence,
                                    const void *data) {
    if (rc->type != RenderResourceType::INDEX) {
        return;
    }
    GLuint handle;
    u32 type_size = get_index_size(type);

    HardwareIndexGL index;
    index.size = element_cnt * type_size;
    index.type = type;
    rc->handle = this->indices.insert(index);
    std::lock_guard lg(alloc_lock);
    void *tmp = nullptr;
    if (data) {
        tmp = malloc(index.size);
        memcpy(tmp, data, index.size);
    }

    this->alloc_cmds.push(
        AllocCommand{.rc = *rc, .is_alloc = true, .alloc_data = tmp});
}

void RenderBackendGL::alloc_constant(RenderResource *rc, u32 size,
                                     const void *data) {
    if (rc->type != RenderResourceType::CONSTANT) {
        return;
    }
    HardwareBufferGL constant;
    constant.size = size;
    rc->handle = this->ubos.insert(constant);
    std::lock_guard lg(alloc_lock);
    void *tmp = nullptr;
    if (data) {
        tmp = malloc(size);
        memcpy(tmp, data, size);
    }

    this->alloc_cmds.push(
        AllocCommand{.rc = *rc, .is_alloc = true, .alloc_data = tmp});
}

void RenderBackendGL::alloc_shader(RenderResource *rc,
                                   const std::string &vertex_code,
                                   const std::string &fragment_code,
                                   const std::string &geometry_code,
                                   const std::string &tess_ctrl_code,
                                   const std::string &tess_eval_code) {
    if (rc->type != RenderResourceType::SHADER) {
        return;
    }
    HardwareShaderGL shader;
    shader.vertex_src = vertex_code;
    shader.fragment_src = fragment_code;
    shader.geo_src = geometry_code;
    shader.tess_ctrl_src = tess_ctrl_code;
    shader.tess_eval_src = tess_eval_code;

    rc->handle = this->shaders.insert(shader);
    std::lock_guard lg(alloc_lock);

    this->alloc_cmds.push(
        AllocCommand{.rc = *rc, .is_alloc = true, .alloc_data = nullptr});
}

void RenderBackendGL::alloc_pipeline(RenderResource *rc, RenderResource shader,
                                     const RenderRasterizerState &rst_state,
                                     const RenderDepthStencilState &depth_state,
                                     const RenderBlendState &blend_state) {
    if (rc->type != RenderResourceType::PIPELINE) {
        return;
    }
    HardwarePipelineGL pl = {.shader = shader,
                             .rst_state = rst_state,
                             .depth_state = depth_state,
                             .blend_state = blend_state};
    std::lock_guard lg(alloc_lock);

    rc->handle = this->pipelines.insert(pl);
}

void RenderBackendGL::alloc_render_target(RenderResource *rc, bool depth_only) {
    rc->handle = this->render_targets.insert({.depth_only = depth_only});
    std::lock_guard lg(alloc_lock);

    this->alloc_cmds.push(
        AllocCommand{.rc = *rc, .is_alloc = true, .alloc_data = nullptr});
}

void RenderBackendGL::alloc_buffer(RenderResource *rc, u32 size,
                                   const void *data) {
    rc->handle = this->ssbos.insert({.size = size});
    std::lock_guard lg(alloc_lock);
    void *tmp = nullptr;
    if (data) {
        tmp = malloc(size);
        memcpy(tmp, data, size);
    }

    this->alloc_cmds.push(
        AllocCommand{.rc = *rc, .is_alloc = true, .alloc_data = tmp});
}

void RenderBackendGL::dealloc(RenderResource *rc) {
    std::lock_guard lg(alloc_lock);

    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = false});
}

void RenderBackendGL::handle_alloc(AllocCommand &cmd) {
    RenderResource rc = cmd.rc;
    switch (rc.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *buffer = this->vertices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(buffer);
            glGenBuffers(1, &buffer->handle);
            glBindBuffer(GL_ARRAY_BUFFER, buffer->handle);
            glBufferData(GL_ARRAY_BUFFER, buffer->size, cmd.alloc_data,
                         GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            break;
        }
        case RenderResourceType::INDEX: {
            HardwareIndexGL *index = this->indices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(index);
            glGenBuffers(1, &index->handle);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->handle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, index->size, cmd.alloc_data,
                         GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            break;
        }
        case RenderResourceType::CONSTANT: {
            HardwareBufferGL *constant = this->ubos.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(constant);

            glGenBuffers(1, &constant->handle);
            glBindBuffer(GL_UNIFORM_BUFFER, constant->handle);
            glBufferData(GL_UNIFORM_BUFFER, constant->size, cmd.alloc_data,
                         GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            break;
        }
        case RenderResourceType::TEXTURE: {
            HardwareTextureGL *tex = this->textures.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(tex);

            GLuint type = GLHelper::texture_type(tex->type);
            GLuint format = GLHelper::pixel_format(tex->format);
            GLuint internal = GLHelper::pixel_internal(tex->format);
            glGenTextures(1, &tex->handle);
            glBindTexture(type, tex->handle);
            if (tex->type == TextureType::TEXTURE_2D_MULTISAMPLE) {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, format,
                                        tex->w, tex->h, GL_TRUE);
                glBindTexture(type, 0);
                break;
            }

            glTexParameteri(type, GL_TEXTURE_WRAP_S,
                            GLHelper::wrap_mode(tex->property.wrap_u));
            glTexParameteri(type, GL_TEXTURE_WRAP_T,
                            GLHelper::wrap_mode(tex->property.wrap_v));
            glTexParameteri(type, GL_TEXTURE_WRAP_R,
                            GLHelper::wrap_mode(tex->property.wrap_w));
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER,
                            GLHelper::filter(tex->property.min_filter));
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER,
                            GLHelper::filter(tex->property.mag_filter));
            if (tex->type == TextureType::TEXTURE_CUBEMAP) {
                /* we don't allocate for cube map*/
                glBindTexture(type, 0);
                break;
            }
            if (tex->format == PixelFormat::D24S8) {
                // depth stencil texture
                glTexImage2D(type, 0, internal, tex->w, tex->h, 0, format,
                             GL_UNSIGNED_INT_24_8, cmd.alloc_data);
            } else {
                // normal texture
                glTexImage2D(type, 0, internal, tex->w, tex->h, 0, format,
                             GL_UNSIGNED_BYTE, cmd.alloc_data);
                // glGenerateMipmap(type);
            }
            glBindTexture(type, 0);
            break;
        }
        case RenderResourceType::RENDER_TARGET: {
            HardwareRenderTargetGL *rt =
                this->render_targets.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(rt);
            glGenFramebuffers(1, &rt->fbo);
            if (rt->depth_only) {
                glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            break;
        }
        case RenderResourceType::BUFFER: {
            HardwareBufferGL *ssbo = this->ssbos.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(ssbo);
            glGenBuffers(1, &ssbo->handle);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo->handle);
            glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo->size, cmd.alloc_data,
                         GL_DYNAMIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            break;
        }
        case RenderResourceType::SHADER: {
            HardwareShaderGL *shader = this->shaders.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(shader);
            u32 vertex, fragment, geometry, tess_ctrl, tess_eval;
            int success;
            char info[512];
            u32 program = glCreateProgram();
            const char *vertex_c = shader->vertex_src.c_str();
            const char *frag_c = shader->fragment_src.c_str();
            const char *geo_c = shader->geo_src.c_str();
            const char *tess_ctrl_c = shader->tess_ctrl_src.c_str();
            const char *tess_eval_c = shader->tess_eval_src.c_str();
            std::vector<std::string> samplers;

            /* vertex shader */
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vertex_c, NULL);
            glCompileShader(vertex);
            glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(vertex, 512, NULL, info);
                spdlog::error("shader source: {}", vertex_c);
                throw std::runtime_error(info);
            }
            glAttachShader(program, vertex);
            GLHelper::find_samplers(shader->vertex_src, samplers);

            /* optional tesselation shader */
            if (shader->tess_ctrl_src.size() > 0 &&
                shader->tess_eval_src.size() > 0) {
                tess_ctrl = glCreateShader(GL_TESS_CONTROL_SHADER);
                tess_eval = glCreateShader(GL_TESS_EVALUATION_SHADER);

                glShaderSource(tess_ctrl, 1, &tess_ctrl_c, NULL);
                glShaderSource(tess_eval, 1, &tess_eval_c, NULL);

                glCompileShader(tess_ctrl);
                glCompileShader(tess_eval);
                glGetShaderiv(tess_ctrl, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(tess_ctrl, 512, NULL, info);
                    spdlog::error("shader source: {}", tess_ctrl_c);
                    throw std::runtime_error(info);
                }
                glGetShaderiv(tess_eval, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(tess_eval, 512, NULL, info);
                    spdlog::error("shader source: {}", tess_eval_c);
                    throw std::runtime_error(info);
                }
                glAttachShader(program, tess_ctrl);
                glAttachShader(program, tess_eval);
                GLHelper::find_samplers(shader->tess_ctrl_src, samplers);
                GLHelper::find_samplers(shader->tess_eval_src, samplers);

            } else if (shader->tess_ctrl_src.size() == 0 &&
                           shader->tess_eval_src.size() > 0 ||
                       shader->tess_ctrl_src.size() > 0 &&
                           shader->tess_eval_src.size() == 0) {
                throw std::runtime_error(
                    "TCS and TES need to be provide at same time.");
            }
            /* optional geometry shader */
            if (shader->geo_src.size() > 0) {
                geometry = glCreateShader(GL_GEOMETRY_SHADER);
                glShaderSource(geometry, 1, &geo_c, NULL);
                glCompileShader(geometry);
                glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(geometry, 512, NULL, info);
                    spdlog::error("shader source: {}", geo_c);
                    throw std::runtime_error(info);
                }
                glAttachShader(program, geometry);
                GLHelper::find_samplers(shader->geo_src, samplers);
            }

            if (shader->fragment_src.size() > 0) {
                /* fragment shader */
                fragment = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragment, 1, &frag_c, NULL);
                glCompileShader(fragment);
                glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(fragment, 512, NULL, info);
                    spdlog::error("shader source: {}", frag_c);
                    throw std::runtime_error(info);
                }
                glAttachShader(program, fragment);
                GLHelper::find_samplers(shader->fragment_src, samplers);
            }

            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(program, 512, NULL, info);
                if (shader->vertex_src.size() > 0)
                    spdlog::info(shader->vertex_src);
                if (shader->tess_ctrl_src.size() > 0)
                    spdlog::info(shader->tess_ctrl_src);
                if (shader->tess_ctrl_src.size() > 0)
                    spdlog::info(shader->tess_ctrl_src);
                if (shader->geo_src.size() > 0) spdlog::info(shader->geo_src);
                if (shader->fragment_src.size() > 0)
                    spdlog::info(shader->fragment_src);
                throw std::runtime_error(info);
            }
            glDeleteShader(vertex);
            if (shader->tess_ctrl_src.size() > 0) glDeleteShader(tess_ctrl);
            if (shader->tess_eval_src.size() > 0) glDeleteShader(tess_eval);
            if (shader->geo_src.size() > 0) glDeleteShader(geometry);
            if (shader->fragment_src.size() > 0) glDeleteShader(fragment);
            shader->handle = program;
            glUseProgram(program);
            /* attach samplers */
            for (i32 i = 0; i < samplers.size(); i++) {
                u32 loc =
                    glGetUniformLocation(shader->handle, samplers[i].c_str());
                glUniform1i(loc, i);
            }
            glUseProgram(0);
            break;
        }

        default:
            break;
    }
    if (cmd.alloc_data) {
        free(cmd.alloc_data);
    }
}

void RenderBackendGL::handle_dealloc(AllocCommand &cmd) {
    RenderResource rc = cmd.rc;
    switch (rc.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *vertices = this->vertices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(vertices);
            glDeleteBuffers(1, &vertices->handle);
            this->vertices.remove(rc.handle);
            break;
        }

        case RenderResourceType::INDEX: {
            HardwareIndexGL *index = this->indices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(index);
            glDeleteBuffers(1, &index->handle);
            this->indices.remove(rc.handle);
            break;
        }
        case RenderResourceType::TEXTURE: {
            HardwareTextureGL *tex = this->textures.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(tex);
            glDeleteBuffers(1, &tex->handle);
            this->textures.remove(rc.handle);
            break;
        }
        case RenderResourceType::SHADER: {
            HardwareShaderGL *shader = this->shaders.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(shader);
            glDeleteProgram(shader->handle);
            this->shaders.remove(rc.handle);
            break;
        }
        case RenderResourceType::PIPELINE: {
            this->pipelines.remove(rc.handle);
            break;
        }
        case RenderResourceType::RENDER_TARGET: {
            HardwareRenderTargetGL *rt =
                this->render_targets.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(rt);
            glDeleteFramebuffers(1, &rt->fbo);
            this->render_targets.remove(rc.handle);
            break;
        }
        default:
            break;
    }
}

void RenderBackendGL::handle_update(RenderCommand &cmd) {
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);
    if (!update_data->filled) {
        this->push_cmd(cmd);
        return;
    }
    RenderResource rc = update_data->rc;
    /* data is right after header */
    void *data = &update_data[1];
    switch (rc.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *vertex = this->vertices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(vertex);
            glBindBuffer(GL_ARRAY_BUFFER, vertex->handle);
            if (vertex->size < update_data->buffer.size) {
                glBufferData(GL_ARRAY_BUFFER, update_data->buffer.size, data,
                             GL_STATIC_DRAW);
                vertex->size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, update_data->buffer.offset,
                                update_data->buffer.size, data);
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            break;
        }
        case RenderResourceType::TEXTURE: {
            HardwareTextureGL *tex = this->textures.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(tex);
            GLuint type = GLHelper::texture_type(tex->type);
            GLuint format = GLHelper::pixel_format(tex->format);
            GLuint internal = GLHelper::pixel_internal(tex->format);
            glBindTexture(type, tex->handle);
            if (tex->type == TextureType::TEXTURE_CUBEMAP) {
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + update_data->texture.face,
                    0, format, update_data->texture.w, update_data->texture.h,
                    0, internal, GL_UNSIGNED_BYTE, data);
            } else {
                glTexSubImage2D(type, 0, update_data->texture.x_off,
                                update_data->texture.y_off,
                                update_data->texture.w, update_data->texture.h,
                                format, GL_UNSIGNED_BYTE, data);
            }

            glBindTexture(type, 0);
            break;
        }
        case RenderResourceType::CONSTANT: {
            HardwareBufferGL *constant = this->ubos.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(constant);

            glBindBuffer(GL_UNIFORM_BUFFER, constant->handle);
            if (constant->size < update_data->buffer.size) {
                glBufferData(GL_UNIFORM_BUFFER, update_data->buffer.size, data,
                             GL_DYNAMIC_DRAW);
                constant->size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_UNIFORM_BUFFER, update_data->buffer.offset,
                                update_data->buffer.size, data);
            }
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            break;
        }
        case RenderResourceType::INDEX: {
            HardwareIndexGL *index = this->indices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(index);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->handle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, update_data->buffer.size,
                         data, GL_STATIC_DRAW);
            index->size = update_data->buffer.size;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            break;
        }
        case RenderResourceType::RENDER_TARGET: {
            HardwareRenderTargetGL *rt =
                this->render_targets.get_or_null(rc.handle);
            EXPECT_NOT_NULL_BREAK(rt);
            HardwareTextureGL *tex = this->textures.get_or_null(
                update_data->attachment.texture.handle);
            EXPECT_NOT_NULL_BREAK(tex);
            glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);
            GLuint slot;
            if (update_data->attachment.is_depth) {
                if (tex->format == PixelFormat::D24) {
                    slot = GL_DEPTH_ATTACHMENT;
                } else if (tex->format == PixelFormat::D24S8) {
                    slot = GL_DEPTH_STENCIL_ATTACHMENT;
                }
            } else {
                slot = GL_COLOR_ATTACHMENT0 + update_data->attachment.slot;
            }
            if (tex->type == TextureType::TEXTURE_CUBEMAP) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, slot,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X +
                                           update_data->attachment.face,
                                       tex->handle, 0);
            } else {
                glFramebufferTexture2D(GL_FRAMEBUFFER, slot,
                                       GLHelper::texture_type(tex->type),
                                       tex->handle, 0);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, last_fbo);
            break;
        }
        case RenderResourceType::BUFFER: {
            HardwareBufferGL *buffer = this->ssbos.get_or_null(rc.handle);
            EXPECT_NOT_NULL_BREAK(buffer);

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->handle);
            if (buffer->size < update_data->buffer.size) {
                glBufferData(GL_SHADER_STORAGE_BUFFER, update_data->buffer.size,
                             data, GL_DYNAMIC_COPY);
                buffer->size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                                update_data->buffer.offset,
                                update_data->buffer.size, data);
            }
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            break;
        }
        default:
            break;
    }
    free(update_data);
}
void RenderBackendGL::use_vertex_desc(VertexLayout *desc) {
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

        switch (attr.type) {
            case VertexAttributeType::UNSIGNED_BYTE:
            case VertexAttributeType::UNSIGNED:
            case VertexAttributeType::INT:
                if (attr.should_normalized) {
                    glVertexAttribPointer(attr.layout_num, attr.size, type,
                                          attr.should_normalized,
                                          desc->get_stride(),
                                          (void *)(size_t)cnt);
                } else {
                    glVertexAttribIPointer(attr.layout_num, attr.size, type,
                                           desc->get_stride(),
                                           (void *)(size_t)cnt);
                }
                break;
            case VertexAttributeType::FLOAT:
            default:
                glVertexAttribPointer(attr.layout_num, attr.size, type,
                                      attr.should_normalized,
                                      desc->get_stride(), (void *)(size_t)cnt);
                break;
        }

        glVertexAttribDivisor(attr.layout_num, desc->is_instance() ? 1 : 0);
        cnt += attr.size * size;
    }
    glEnableVertexAttribArray(0);
}

void RenderBackendGL::bind_buffer(RenderResource &rc) {
    switch (rc.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *hb = this->vertices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(hb);
            glBindBuffer(GL_ARRAY_BUFFER, hb->handle);
            break;
        }
        case RenderResourceType::INDEX: {
            HardwareBufferGL *hb = this->indices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(hb);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hb->handle);
            break;
        }
        default:
            break;
    }
}

void RenderBackendGL::use_shader(RenderResource &rc) {
    HardwareShaderGL *shader = this->shaders.get_or_null(rc.handle);
    EXPECT_NOT_NULL_RET(shader);
    glUseProgram(shader->handle);
}
void RenderBackendGL::use_texture(u32 unit, RenderResource &rc) {
    HardwareTextureGL *tex = this->textures.get_or_null(rc.handle);
    EXPECT_NOT_NULL_RET(tex);
    GLuint type = GLHelper::texture_type(tex->type);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(type, tex->handle);
}

void RenderBackendGL::setup_rasterizer(const RenderRasterizerState &state) {
    switch (state.cull_mode) {
        case Cullmode::FRONT:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case Cullmode::BACK:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case Cullmode::BOTH:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT_AND_BACK);
            break;
        case Cullmode::NONE:
        default:
            glDisable(GL_CULL_FACE);
            break;
    }
    switch (state.poly_mode) {
        case PolygonMode::POINT:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case PolygonMode::LINE:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case PolygonMode::FILL:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        default:
            break;
    }
    glPatchParameteri(GL_PATCH_VERTICES, state.patch_control_points);
}

void RenderBackendGL::setup_depth_stencil(
    const RenderDepthStencilState &state) {
    if (state.depth_on)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (state.stencil_on)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
    glDepthFunc(GLHelper::compare_op(state.depth_compare_op));
    glStencilFunc(GLHelper::compare_op(state.stencil_compare_op), 1, 0xff);
}

void RenderBackendGL::setup_blend(const RenderBlendState &state) {
    if (state.blend_on) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GLHelper::blend_factor(state.func.src_rgb),
                            GLHelper::blend_factor(state.func.dst_rgb),
                            GLHelper::blend_factor(state.func.src_alpha),
                            GLHelper::blend_factor(state.func.dst_alpha));
    } else
        glDisable(GL_BLEND);
}

void RenderBackendGL::handle_state(RenderCommand &cmd) {
    RenderStateData *state_data = static_cast<RenderStateData *>(cmd.data);
    RenderStateData::Operation *head =
        (RenderStateData::Operation *)(((u64)state_data) +
                                       sizeof(RenderStateData));

    for (i32 i = 0; i < state_data->operation_cnt; i++) {
        auto *op = &head[i];
        auto type = op->type;
        switch (type) {
            case RenderStateData::OpType::CLEAR: {
                GLuint clear_flag = 0;
                if (op->clear_flag & StateClearFlag::CLEAR_COLOR) {
                    clear_flag |= GL_COLOR_BUFFER_BIT;
                }
                if (op->clear_flag & StateClearFlag::CLEAR_DEPTH) {
                    clear_flag |= GL_DEPTH_BUFFER_BIT;
                }
                if (op->clear_flag & StateClearFlag::CLEAR_STENCIL) {
                    clear_flag |= GL_STENCIL_BUFFER_BIT;
                }
                glClear(clear_flag);
                break;
            }
            case RenderStateData::OpType::VIEWPORT: {
                glViewportArrayv(0, op->viewports.counts,
                                 (const GLfloat *)op->viewports.view_rects);
                break;
            }
            case RenderStateData::OpType::SCISSOR: {
                RectF &rect = op->scissor_rect;
                glEnable(GL_SCISSOR_TEST);
                glScissor(rect.x, rect.y, rect.w, rect.h);
                break;
            }
            case RenderStateData::OpType::BIND_RENDER_TARGET: {
                if (op->render_target.type ==
                    RenderResourceType::UNINITIALIZE) {
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    last_fbo = 0;
                    break;
                }
                HardwareRenderTargetGL *render_target =
                    this->render_targets.get_or_null(op->render_target.handle);
                EXPECT_NOT_NULL_BREAK(render_target);

                glBindFramebuffer(GL_FRAMEBUFFER, render_target->fbo);

                last_fbo = render_target->fbo;
                auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    SPDLOG_ERROR("Incomplete framebuffer code: {}", status);
                }
                break;
            }
            case RenderStateData::OpType::BIND_BUFFERBASE: {
                HardwareBufferGL *buffer = nullptr;
                if (op->bufferbase.buffer.type == RenderResourceType::BUFFER) {
                    this->ssbos.get_or_null(op->bufferbase.buffer.handle);
                } else if (op->bufferbase.buffer.type ==
                           RenderResourceType::CONSTANT) {
                    this->ubos.get_or_null(op->bufferbase.buffer.handle);
                }
                EXPECT_NOT_NULL_BREAK(buffer);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->handle);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, op->bufferbase.base,
                                 buffer->handle);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
                break;
            }
            default:
                break;
        }
    }
}

void RenderBackendGL::handle_render(RenderCommand &cmd) {
    RenderDrawData *draw_data = static_cast<RenderDrawData *>(cmd.data);
    /* bind pipeline */
    if (!this->current_pipeline.inited() ||
        this->current_pipeline.handle != draw_data->pipeline.handle) {
        HardwarePipelineGL *pl =
            pipelines.get_or_null(draw_data->pipeline.handle);
        EXPECT_NOT_NULL_RET(pl);
        this->current_pipeline = draw_data->pipeline;
        use_shader(pl->shader);
        setup_rasterizer(pl->rst_state);
        setup_depth_stencil(pl->depth_state);
        setup_blend(pl->blend_state);
    }
    RenderDrawData::Operation *head =
        (RenderDrawData::Operation *)(((u64)draw_data) +
                                      sizeof(RenderDrawData));
    u32 index_type = 0;

    for (i32 i = 0; i < draw_data->operation_cnt; i++) {
        auto *op = &head[i];
        auto type = op->type;
        switch (type) {
            case RenderDrawData::OpType::BIND_VERTEX:
                bind_buffer(op->vertex_rc);
                break;
            case RenderDrawData::OpType::BIND_INDEX: {
                bind_buffer(op->index_rc);
                HardwareIndexGL *index =
                    this->indices.get_or_null(op->index_rc.handle);
                switch (index->type) {
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
                break;
            }
            case RenderDrawData::OpType::BIND_DESC:
                use_vertex_desc(op->vertex_desc);
                break;
            case RenderDrawData::OpType::BIND_TEXTURE:
                use_texture(op->texture.unit, op->texture.rc);
                break;
            case RenderDrawData::OpType::VIEWPORT: {
                RectF &vp = op->view_rect;
                glViewport(vp.x, vp.y, vp.w, vp.h);
                break;
            }
            case RenderDrawData::OpType::SCISSOR: {
                RectF &rect = op->scissor_rect;
                glEnable(GL_SCISSOR_TEST);
                glScissor(rect.x, rect.y, rect.w, rect.h);
                break;
            }
            case RenderDrawData::OpType::PUSH_CONSTANT: {
                HardwareBufferGL *pc = ubos.get_or_null(push_constant.handle);
                glBindBuffer(GL_UNIFORM_BUFFER, pc->handle);
                glBufferData(GL_UNIFORM_BUFFER, op->constant.size,
                             op->constant.data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_UNIFORM_BUFFER, 0);
                break;
            }
            default:
                break;
        }
    }

    /* select primitive to draw */
    u32 prim_type;
    switch (draw_data->type) {
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

    if (index_type != 0) {
        if (draw_data->instance_cnt > 0) {
            glDrawElementsInstancedBaseInstance(
                prim_type, draw_data->vertex_cnt, index_type,
                (void *)(u64)draw_data->index_offset, draw_data->instance_cnt,
                draw_data->instance_offset);
        } else {
            glDrawElements(prim_type, draw_data->vertex_cnt, index_type,
                           (void *)(u64)draw_data->index_offset);
        }
    } else {
        if (draw_data->instance_cnt > 0) {
            glDrawArraysInstancedBaseInstance(
                prim_type, draw_data->vertex_offset, draw_data->vertex_cnt,
                draw_data->instance_cnt, draw_data->instance_offset);
        } else {
            glDrawArrays(prim_type, draw_data->vertex_offset,
                         draw_data->vertex_cnt);
        }
    }
}

void RenderBackendGL::process_commands(std::deque<RenderCommand> &cmd_queue) {
    /* alloc resources first */
    while (!alloc_cmds.empty()) {
        AllocCommand &cmd = alloc_cmds.front();
        if (cmd.is_alloc) {
            handle_alloc(cmd);
        } else {
            handle_dealloc(cmd);
        }
        alloc_cmds.pop();
    }
    while (!cmd_queue.empty()) {
        RenderCommand &cmd = cmd_queue.front();
        switch (cmd.type) {
            case RenderCommandType::UPDATE:
                handle_update(cmd);
                break;
            case RenderCommandType::STATE:
                handle_state(cmd);
                break;
            case RenderCommandType::RENDER:
                handle_render(cmd);
                break;
            case RenderCommandType::BEGIN_SCOPE:
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1,
                                 (const GLchar *)cmd.data);
                break;
            case RenderCommandType::END_SCOPE:
                glPopDebugGroup();
                break;
            default:
                break;
        }
        cmd_queue.pop_front();
    }
}
void RenderBackendGL::swap_buffer() {
    glfwSwapBuffers(glfwGetCurrentContext());
}
}  // namespace Seed