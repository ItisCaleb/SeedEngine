#include "render_device_opengl.h"
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
void RenderDeviceOpenGL::alloc_texture(RenderResource *rc, TextureType type,
                                       u32 w, u32 h) {
    HardwareTextureGL texture;
    texture.w = w;
    texture.h = h;
    texture.type = type;
    rc->handle = this->textures.insert(texture);
    this->alloc_cmds.push(AllocCommand{.handle = rc->handle,
                                       .type = RenderResourceType::TEXTURE,
                                       .is_alloc = true});
}
void RenderDeviceOpenGL::alloc_vertex(RenderResource *rc, u32 stride,
                                      u32 vertex_cnt) {
    HardwareBufferGL buffer;
    buffer.size = stride * vertex_cnt;
    rc->handle = this->buffers.insert(buffer);
    this->alloc_cmds.push(AllocCommand{.handle = rc->handle,
                                       .type = RenderResourceType::VERTEX,
                                       .is_alloc = true});
}

void RenderDeviceOpenGL::alloc_indices(RenderResource *rc, IndexType type,
                                       u32 element_cnt) {
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
    this->alloc_cmds.push(AllocCommand{.handle = rc->handle,
                                       .type = RenderResourceType::INDEX,
                                       .is_alloc = true});
}

void RenderDeviceOpenGL::alloc_constant(RenderResource *rc,
                                        const std::string &name, u32 size) {
    HardwareConstantGL constant;
    constant.size = size;
    constant.name = name;
    rc->handle = this->constants.insert(constant);
    this->constants.get_or_null(rc->handle)->buffer_base = rc->handle;
    this->alloc_cmds.push(AllocCommand{.handle = rc->handle,
                                       .type = RenderResourceType::CONSTANT,
                                       .is_alloc = true});
}

void RenderDeviceOpenGL::alloc_shader(RenderResource *rc,
                                      const std::string &vertex_code,
                                      const std::string &fragment_code,
                                      const std::string &geometry_code,
                                      const std::string &tess_ctrl_code,
                                      const std::string &tess_eval_code) {
    HardwareShaderGL shader;
    shader.vertex_src = vertex_code;
    shader.fragment_src = fragment_code;
    shader.geo_src = geometry_code;
    shader.tess_ctrl_src = tess_ctrl_code;
    shader.tess_eval_src = tess_eval_code;

    rc->handle = this->shaders.insert(shader);
    this->alloc_cmds.push(AllocCommand{.handle = rc->handle,
                                       .type = RenderResourceType::SHADER,
                                       .is_alloc = true});
    this->shader_in_use.push_back(rc->handle);
}

void RenderDeviceOpenGL::alloc_pipeline(
    RenderResource *rc, RenderResource shader,
    const RenderRasterizerState &rst_state,
    const RenderDepthStencilState &depth_state,
    const RenderBlendState &blend_state) {
    HardwarePipelineGL pl = {.shader = shader,
                             .rst_state = rst_state,
                             .depth_state = depth_state,
                             .blend_state = blend_state};
    rc->handle = this->pipelines.insert(pl);
}

void RenderDeviceOpenGL::alloc_render_target(RenderResource *rc) {
    rc->handle = this->render_targets.insert({});
    this->alloc_cmds.push(
        AllocCommand{.handle = rc->handle,
                     .type = RenderResourceType::RENDER_TARGET,
                     .is_alloc = true});
}

void RenderDeviceOpenGL::dealloc(RenderResource *r) {
    this->alloc_cmds.push(
        AllocCommand{.handle = r->handle, .type = r->type, .is_alloc = false});
}
void RenderDeviceOpenGL::find_samplers(const std::string &src,
                                       std::vector<std::string> &result) {
    std::regex sampler_regex(
        R"(\b(?:uniform\s+)?sampler\w*\s+(\w+)(\s*\[\s*(\d+)\s*\])?)");
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

void RenderDeviceOpenGL::handle_alloc(AllocCommand &cmd) {
    switch (cmd.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *buffer = this->buffers.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(buffer);
            glGenBuffers(1, &buffer->handle);
            glBindBuffer(GL_ARRAY_BUFFER, buffer->handle);
            glBufferData(GL_ARRAY_BUFFER, buffer->size, nullptr,
                         GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            break;
        }
        case RenderResourceType::INDEX: {
            HardwareIndexGL *index = this->indices.get_or_null(cmd.handle);
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
                this->constants.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(constant);

            glGenBuffers(1, &constant->handle);
            glBindBuffer(GL_UNIFORM_BUFFER, constant->handle);
            glBufferData(GL_UNIFORM_BUFFER, constant->size, nullptr,
                         GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, cmd.handle, constant->handle);
            constant->buffer_base = cmd.handle;
            for (Handle handle : shader_in_use) {
                HardwareShaderGL *shader = this->shaders.get_or_null(handle);
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
            HardwareTextureGL *tex = this->textures.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(tex);

            GLuint type = convert_texture_type(tex->type);
            glGenTextures(1, &tex->handle);
            glBindTexture(type, tex->handle);
            glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            if (tex->type == TextureType::TEXTURE_CUBEMAP) {
                /* we don't allocate for cube map*/
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                                GL_CLAMP_TO_EDGE);
            } else {
                glTexImage2D(type, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, nullptr);
            }
            glGenerateMipmap(type);
            glBindTexture(type, 0);
            break;
        }
        case RenderResourceType::RENDER_TARGET: {
            HardwareRenderTargetGL *rt = this->render_targets.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(rt);
            glGenFramebuffers(1, &rt->handle);
        }
        case RenderResourceType::SHADER: {
            HardwareShaderGL *shader = this->shaders.get_or_null(cmd.handle);
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
                throw std::runtime_error(info);
            }
            glAttachShader(program, vertex);
            find_samplers(shader->vertex_src, samplers);

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
            find_samplers(shader->fragment_src, samplers);

            /* optional geometry shader */
            if (shader->geo_src.size() > 0) {
                geometry = glCreateShader(GL_GEOMETRY_SHADER);
                glShaderSource(geometry, 1, &geo_c, NULL);
                glCompileShader(geometry);
                glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(geometry, 512, NULL, info);
                    throw std::runtime_error(info);
                }
                glAttachShader(program, geometry);
                find_samplers(shader->geo_src, samplers);
            }

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
                    throw std::runtime_error(info);
                }
                glGetShaderiv(tess_eval, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(tess_ctrl, 512, NULL, info);
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

            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(program, 512, NULL, info);
                throw std::runtime_error(info);
            }
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            shader->handle = program;
            for (i32 i = 0; i < samplers.size(); i++) {
                u32 loc =
                    glGetUniformLocation(shader->handle, samplers[i].c_str());
                glUniform1i(loc, i);
            }
            break;
        }

        default:
            break;
    }
}

GLuint RenderDeviceOpenGL::convert_texture_type(TextureType type) {
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
        default:
            break;
    }
    return t;
}

void RenderDeviceOpenGL::handle_dealloc(AllocCommand &cmd) {
    switch (cmd.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *buffer = this->buffers.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(buffer);
            glDeleteBuffers(1, &buffer->handle);
            this->buffers.remove(cmd.handle);
        }

        case RenderResourceType::INDEX: {
            HardwareIndexGL *index = this->indices.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(index);
            glDeleteBuffers(1, &index->handle);
            this->indices.remove(cmd.handle);
            break;
        }
        case RenderResourceType::TEXTURE: {
            HardwareTextureGL *tex = this->textures.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(tex);
            glDeleteBuffers(1, &tex->handle);
            this->textures.remove(cmd.handle);
            break;
        }
        case RenderResourceType::SHADER: {
            HardwareShaderGL *shader = this->shaders.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(shader);
            glDeleteProgram(shader->handle);
            this->shaders.remove(cmd.handle);
            break;
        }
        case RenderResourceType::PIPELINE: {
            this->pipelines.remove(cmd.handle);
            break;
        }
        case RenderResourceType::RENDER_TARGET: {
            HardwareRenderTargetGL *rt =
                this->render_targets.get_or_null(cmd.handle);
            EXPECT_NOT_NULL_RET(rt);
            glDeleteFramebuffers(1, &rt->handle);
        }
        default:
            break;
    }
}

void RenderDeviceOpenGL::handle_update(RenderCommand &cmd) {
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);
    RenderResource buffer = update_data->rc;
    switch (buffer.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *hb = this->buffers.get_or_null(buffer.handle);
            EXPECT_NOT_NULL_RET(hb);

            glBindBuffer(GL_ARRAY_BUFFER, hb->handle);
            if (hb->size < update_data->buffer.size) {
                glBufferData(GL_ARRAY_BUFFER, update_data->buffer.size,
                             update_data->data, GL_DYNAMIC_DRAW);
                hb->size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, update_data->buffer.offset,
                                update_data->buffer.size, update_data->data);
            }
            break;
        }
        case RenderResourceType::TEXTURE: {
            HardwareTextureGL *tex = this->textures.get_or_null(buffer.handle);
            EXPECT_NOT_NULL_RET(tex);
            GLuint type = convert_texture_type(tex->type);
            glBindTexture(type, tex->handle);
            if (tex->type == TextureType::TEXTURE_CUBEMAP) {
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + update_data->texture.face,
                    0, GL_RGBA, update_data->texture.w, update_data->texture.h,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, update_data->data);
            } else {
                glTexSubImage2D(type, 0, update_data->texture.x_off,
                                update_data->texture.y_off,
                                update_data->texture.w, update_data->texture.h,
                                GL_RGBA, GL_UNSIGNED_BYTE, update_data->data);
            }

            glBindTexture(type, 0);
            break;
        }
        case RenderResourceType::CONSTANT: {
            HardwareConstantGL *constant =
                this->constants.get_or_null(buffer.handle);
            EXPECT_NOT_NULL_RET(constant);

            glBindBuffer(GL_UNIFORM_BUFFER, constant->handle);
            if (constant->size < update_data->buffer.size) {
                glBufferData(GL_UNIFORM_BUFFER, update_data->buffer.size,
                             update_data->data, GL_DYNAMIC_DRAW);
                constant->size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_UNIFORM_BUFFER, update_data->buffer.offset,
                                update_data->buffer.size, update_data->data);
            }
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            break;
        }
        case RenderResourceType::INDEX: {
            HardwareIndexGL *index = this->indices.get_or_null(buffer.handle);
            EXPECT_NOT_NULL_RET(index);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->handle);
            if (index->size < update_data->buffer.size) {
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, update_data->buffer.size,
                             update_data->data, GL_DYNAMIC_DRAW);
                index->size = update_data->buffer.size;
            } else {
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                                update_data->buffer.offset,
                                update_data->buffer.size, update_data->data);
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            break;
        }
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

void RenderDeviceOpenGL::bind_buffer(RenderResource &rc) {
    switch (rc.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferGL *hb = this->buffers.get_or_null(rc.handle);
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

void RenderDeviceOpenGL::use_shader(RenderResource &rc) {
    HardwareShaderGL *shader = this->shaders.get_or_null(rc.handle);
    EXPECT_NOT_NULL_RET(shader);
    glUseProgram(shader->handle);
}
void RenderDeviceOpenGL::use_texture(u32 unit, RenderResource &rc) {
    HardwareTextureGL *tex = this->textures.get_or_null(rc.handle);
    EXPECT_NOT_NULL_RET(tex);
    GLuint type = convert_texture_type(tex->type);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(type, tex->handle);
}

void RenderDeviceOpenGL::setup_rasterizer(const RenderRasterizerState &state) {
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

void RenderDeviceOpenGL::setup_depth_stencil(
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
    }
}
void RenderDeviceOpenGL::setup_blend(const RenderBlendState &state) {
    if (state.blend_on) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(get_blend_func(state.func.src_rgb),
                            get_blend_func(state.func.dst_rgb),
                            get_blend_func(state.func.src_alpha),
                            get_blend_func(state.func.dst_alpha));
    } else
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
        /* convert coordinate system */
        Window *window = RenderEngine::get_instance()->get_current_window();
        ViewportState &vp = state_data->viewport;
        glViewport(vp.x, window->get_height() - vp.y - vp.h, vp.w, vp.h);
    }
    if (flag & RenderStateFlag::SCISSOR) {
        ScissorRect &rect = state_data->rect;
        glEnable(GL_SCISSOR_TEST);
        glScissor(rect.x, rect.y, rect.w, rect.h);
    }
}

void RenderDeviceOpenGL::handle_render(RenderCommand &cmd) {
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
    DrawOperation *head =
        (DrawOperation *)(((u64)draw_data) + sizeof(RenderDrawData));
    u32 index_type = 0;

    for (i32 i = 0; i < draw_data->operation_cnt; i++) {
        DrawOperation *op = &head[i];
        DrawOperationType type = op->type;
        switch (type) {
            case DrawOperationType::BIND_VERTEX:
                bind_buffer(op->vertex.rc);
                break;
            case DrawOperationType::BIND_INDEX: {
                bind_buffer(op->index.rc);
                HardwareIndexGL *index =
                    this->indices.get_or_null(op->index.rc.handle);
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
            case DrawOperationType::BIND_DESC:
                use_vertex_desc(op->desc.desc);
                break;
            case DrawOperationType::BIND_TEXTURE:
                use_texture(op->texure.unit, op->texure.rc);
                break;
            case DrawOperationType::VIEWPORT: {
                ViewportState &vp = op->viewport.rect;
                glViewport(vp.x, vp.y, vp.w, vp.h);
                break;
            }
            case DrawOperationType::SCISSOR: {
                ScissorRect &rect = op->scissor.rect;
                glEnable(GL_SCISSOR_TEST);
                glScissor(rect.x, rect.y, rect.w, rect.h);
                break;
            }
            case DrawOperationType::UPDATE_CONSTANT: {
                HardwareConstantGL *constant =
                    this->constants.get_or_null(op->constant.rc.handle);
                if (!constant) {
                    SPDLOG_ERROR("Constant is null.");
                    break;
                }
                glBindBuffer(GL_UNIFORM_BUFFER, constant->handle);
                if (constant->size < op->constant.size) {
                    glBufferData(GL_UNIFORM_BUFFER, op->constant.size,
                                 op->constant.data, GL_DYNAMIC_DRAW);
                    constant->size = op->constant.size;
                } else {
                    glBufferSubData(GL_UNIFORM_BUFFER, op->constant.offset,
                                    op->constant.size, op->constant.data);
                }
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

void RenderDeviceOpenGL::process() {
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
    std::stable_sort(cmd_queue.begin(), cmd_queue.end(), RenderCommand::cmp);
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
            default:
                break;
        }
        cmd_queue.pop_front();
    }
}
}  // namespace Seed