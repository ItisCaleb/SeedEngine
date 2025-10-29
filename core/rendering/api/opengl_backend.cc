#include "opengl_backend.h"
#include "render_engine.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include "core/macro.h"
#include <regex>

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
    this->alloc_constant(&push_constant, "PushConstant", 0);
}
void RenderBackendGL::alloc_texture(RenderResource *rc, TextureType type, u32 w,
                                    u32 h, PixelFormat format) {
    if (rc->type != RenderResourceType::TEXTURE) {
        return;
    }
    HardwareTextureGL texture;
    texture.w = w;
    texture.h = h;
    texture.type = type;
    texture.format = format;
    rc->handle = this->textures.insert(texture);
    std::lock_guard lg(alloc_lock);
    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = true});
}
void RenderBackendGL::alloc_vertex(RenderResource *rc, u32 stride,
                                   u32 vertex_cnt) {
    if (rc->type != RenderResourceType::VERTEX) {
        return;
    }
    HardwareBufferGL buffer;
    buffer.size = stride * vertex_cnt;
    rc->handle = this->vertices.insert(buffer);
    std::lock_guard lg(alloc_lock);

    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = true});
}

void RenderBackendGL::alloc_indices(RenderResource *rc, IndexType type,
                                    u32 element_cnt) {
    if (rc->type != RenderResourceType::INDEX) {
        return;
    }
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
    HardwareIndexGL index;
    index.size = element_cnt * type_size;
    index.type = type;
    rc->handle = this->indices.insert(index);
    std::lock_guard lg(alloc_lock);

    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = true});
}

void RenderBackendGL::alloc_constant(RenderResource *rc,
                                     const std::string &name, u32 size) {
    if (rc->type != RenderResourceType::CONSTANT) {
        return;
    }
    HardwareConstantGL constant;
    constant.size = size;
    constant.name = name;
    rc->handle = this->constants.insert(constant);
    this->constants.get_or_null(rc->handle)->buffer_base = rc->handle;
    std::lock_guard lg(alloc_lock);

    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = true});
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

    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = true});
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

    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = true});
}

void RenderBackendGL::alloc_buffer(RenderResource *rc, u32 size) {
    rc->handle = this->ssbos.insert({.size = size});
    std::lock_guard lg(alloc_lock);

    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = true});
}

void RenderBackendGL::dealloc(RenderResource *rc) {
    std::lock_guard lg(alloc_lock);

    this->alloc_cmds.push(AllocCommand{.rc = *rc, .is_alloc = false});
}
void RenderBackendGL::find_samplers(const std::string &src,
                                    std::vector<std::string> &result) {
    std::regex sampler_regex(
        R"(\buniform\s+sampler\w*\s+(\w+)(\s*\[\s*(\d+)\s*\])?)");
    std::smatch match;

    std::string::const_iterator search_start(src.cbegin());
    while (std::regex_search(search_start, src.cend(), match, sampler_regex)) {
        /* retrive name */
        std::string name = match[1];
        /* check if is array */
        std::string array_size_str = match[3];

        if (!array_size_str.empty()) {
            i32 array_size = std::stoi(array_size_str);
            for (i32 i = 0; i < array_size; ++i) {
                result.push_back(fmt::format("{}[{}]", name, i));
            }
        } else {
            result.push_back(name);
        }
        search_start = match.suffix().first;
    }
}

void RenderBackendGL::handle_alloc(AllocCommand &cmd) {
    RenderResource rc = cmd.rc;
    switch (rc.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *buffer = this->vertices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(buffer);
            glGenBuffers(1, &buffer->handle);
            glBindBuffer(GL_ARRAY_BUFFER, buffer->handle);
            glBufferData(GL_ARRAY_BUFFER, buffer->size, nullptr,
                         GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            break;
        }
        case RenderResourceType::INDEX: {
            HardwareIndexGL *index = this->indices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(index);
            glGenBuffers(1, &index->handle);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->handle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, index->size, nullptr,
                         GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            break;
        }
        case RenderResourceType::CONSTANT: {
            HardwareConstantGL *constant =
                this->constants.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(constant);

            glGenBuffers(1, &constant->handle);
            glBindBuffer(GL_UNIFORM_BUFFER, constant->handle);
            glBufferData(GL_UNIFORM_BUFFER, constant->size, nullptr,
                         GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, rc.handle, constant->handle);
            constant->buffer_base = rc.handle;

            /* attach uniform buffers */
            std::vector<HardwareShaderGL *> shader_list;
            shaders.get_used(shader_list);
            for (HardwareShaderGL *shader : shader_list) {
                if (shader->handle == GL_INVALID_INDEX) continue;
                u32 idx = glGetUniformBlockIndex(shader->handle,
                                                 constant->name.c_str());
                if (idx != GL_INVALID_INDEX) {
                    glUniformBlockBinding(shader->handle, idx,
                                          constant->buffer_base);
                }
            }
            break;
        }
        case RenderResourceType::TEXTURE: {
            HardwareTextureGL *tex = this->textures.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(tex);

            GLuint type = convert_texture_type(tex->type);
            GLuint format = convert_pixel_format(tex->format);
            GLuint internal = convert_pixel_internal(tex->format);
            glGenTextures(1, &tex->handle);
            glBindTexture(type, tex->handle);
            if (tex->type == TextureType::TEXTURE_CUBEMAP) {
                /* we don't allocate for cube map*/
                glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                                GL_CLAMP_TO_EDGE);
            } else if (tex->type == TextureType::TEXTURE_2D_MULTISAMPLE) {
                // multisample
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, format,
                                        tex->w, tex->h, GL_TRUE);
            } else {
                if (tex->format == PixelFormat::D24S8) {
                    // depth stencil texture
                    glTexImage2D(type, 0, internal, tex->w, tex->h, 0, format,
                                 GL_UNSIGNED_INT_24_8, nullptr);
                } else if (tex->format == PixelFormat::D24) {
                    glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexImage2D(type, 0, internal, tex->w, tex->h, 0, format,
                                 GL_UNSIGNED_BYTE, nullptr);
                } else {
                    // normal texture
                    glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexImage2D(type, 0, internal, tex->w, tex->h, 0, format,
                                 GL_UNSIGNED_BYTE, nullptr);
                    glGenerateMipmap(type);
                }
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
            glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo->size, nullptr,
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
            find_samplers(shader->vertex_src, samplers);

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
                find_samplers(shader->tess_ctrl_src, samplers);
                find_samplers(shader->tess_eval_src, samplers);

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
                find_samplers(shader->geo_src, samplers);
            }

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
            find_samplers(shader->fragment_src, samplers);

            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(program, 512, NULL, info);
                throw std::runtime_error(info);
            }
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            shader->handle = program;
            glUseProgram(program);
            /* attach samplers */
            for (i32 i = 0; i < samplers.size(); i++) {
                u32 loc =
                    glGetUniformLocation(shader->handle, samplers[i].c_str());
                glUniform1i(loc, i);
            }

            /* attach uniform buffers */
            std::vector<HardwareConstantGL *> constant_list;
            constants.get_used(constant_list);
            for (HardwareConstantGL *constant : constant_list) {
                u32 idx = glGetUniformBlockIndex(shader->handle,
                                                 constant->name.c_str());
                if (idx != GL_INVALID_INDEX) {
                    glUniformBlockBinding(shader->handle, idx,
                                          constant->buffer_base);
                }
            }
            glUseProgram(0);
            break;
        }

        default:
            break;
    }
}

GLuint RenderBackendGL::convert_texture_type(TextureType type) {
    GLuint t;
    switch (type) {
        case TextureType::TEXTURE_1D:
            t = GL_TEXTURE_1D;
            break;
        case TextureType::TEXTURE_2D:
            t = GL_TEXTURE_2D;
            break;
        case TextureType::TEXTURE_3D:
            t = GL_TEXTURE_3D;
            break;
        case TextureType::TEXTURE_CUBEMAP:
            t = GL_TEXTURE_CUBE_MAP;
            break;
        case TextureType::TEXTURE_2D_ARRAY:
            t = GL_TEXTURE_2D_ARRAY;
            break;
        case TextureType::TEXTURE_2D_MULTISAMPLE:
            t = GL_TEXTURE_2D_MULTISAMPLE;
            break;
        default:
            break;
    }
    return t;
}

GLuint RenderBackendGL::convert_pixel_internal(PixelFormat format) {
    GLuint t;
    switch (format) {
        case PixelFormat::R:
            t = GL_R8;
            break;
        case PixelFormat::RG:
            t = GL_RG;
            break;
        case PixelFormat::RGB:
            t = GL_RGB;
            break;
        case PixelFormat::RGBA:
            t = GL_RGBA;
            break;
        case PixelFormat::RGBA16F:
            t = GL_RGBA16F;
            break;
        case PixelFormat::D24:
            t = GL_DEPTH_COMPONENT24;
            break;
        case PixelFormat::D24S8:
            t = GL_DEPTH24_STENCIL8;
            break;
        default:
            break;
    }
    return t;
}

GLuint RenderBackendGL::convert_pixel_format(PixelFormat format) {
    GLuint t;
    switch (format) {
        case PixelFormat::R:
            t = GL_RED;
            break;
        case PixelFormat::RG:
            t = GL_RG;
            break;
        case PixelFormat::RGB:
            t = GL_RGB;
            break;
        case PixelFormat::RGBA16F:
        case PixelFormat::RGBA:
            t = GL_RGBA;
            break;
        case PixelFormat::D24:
            t = GL_DEPTH_COMPONENT;
            break;
        case PixelFormat::D24S8:
            t = GL_DEPTH_STENCIL;
            break;
        default:
            break;
    }
    return t;
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
                             GL_DYNAMIC_DRAW);
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
            GLuint type = convert_texture_type(tex->type);
            GLuint format = convert_pixel_format(tex->format);
            GLuint internal = convert_pixel_internal(tex->format);
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
            HardwareConstantGL *constant =
                this->constants.get_or_null(rc.handle);
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
                         data, GL_DYNAMIC_DRAW);
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
            if (update_data->attachment.slot == -1) {
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
                                       convert_texture_type(tex->type),
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

        glVertexAttribDivisor(attr.layout_num, attr.instance_step);
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
    GLuint type = convert_texture_type(tex->type);
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

inline static u32 get_op(CompareOP op) {
    switch (op) {
        case CompareOP::NEVER:
            return GL_NEVER;
        case CompareOP::LESS:
            return GL_LESS;
        case CompareOP::EQUAL:
            return GL_EQUAL;
        case CompareOP::LESS_OR_EQUAL:
            return GL_LEQUAL;
        case CompareOP::GREATER:
            return GL_GREATER;
        case CompareOP::GREATER_OR_EQUAL:
            return GL_GEQUAL;
        case CompareOP::NOT_EQUAL:
            return GL_NOTEQUAL;
        case CompareOP::ALWAYS:
            return GL_ALWAYS;
    }
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
    glDepthFunc(get_op(state.depth_compare_op));
    glStencilFunc(get_op(state.stencil_compare_op), 1, 0xff);
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
        default:
            return GL_ZERO;
    }
}
void RenderBackendGL::setup_blend(const RenderBlendState &state) {
    if (state.blend_on) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(get_blend_func(state.func.src_rgb),
                            get_blend_func(state.func.dst_rgb),
                            get_blend_func(state.func.src_alpha),
                            get_blend_func(state.func.dst_alpha));
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
                HardwareBufferGL *buffer =
                    this->ssbos.get_or_null(op->bufferbase.buffer.handle);
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
                HardwareConstantGL *pc = constants.get_or_null(push_constant.handle); 
                glBindBuffer(GL_UNIFORM_BUFFER, pc->handle);
                glBufferData(GL_UNIFORM_BUFFER, op->constant.size, op->constant.data,
                             GL_DYNAMIC_DRAW);
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
            glDrawElementsInstanced(
                prim_type, draw_data->vertex_cnt, index_type,
                (void *)(u64)draw_data->index_offset, draw_data->instance_cnt);
        } else {
            glDrawElements(prim_type, draw_data->vertex_cnt, index_type,
                           (void *)(u64)draw_data->index_offset);
        }
    } else {
        if (draw_data->instance_cnt > 0) {
            glDrawArraysInstanced(prim_type, draw_data->vertex_offset,
                                  draw_data->vertex_cnt,
                                  draw_data->instance_cnt);
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
}  // namespace Seed