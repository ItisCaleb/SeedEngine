#ifndef _SEED_RENDERING_RESOURCE_H_
#define _SEED_RENDERING_RESOURCE_H_
#include "core/types.h"
#include <vector>
#include <string>
#include <map>
#include "core/rendering/api/render_common.h"
#include "core/handle.h"

namespace Seed {
enum class RenderResourceType: u8 {
    TEXTURE,
    VERTEX,
    INDEX,
    CONSTANT,
    SHADER,
    RENDER_TARGET,
    UNINITIALIZE
};


struct RenderResource {
    Handle handle;
    RenderResourceType type = RenderResourceType::UNINITIALIZE;

    void alloc_texture(TextureType type, u32 w, u32 h, const void *data);
    void alloc_vertex(u32 stride, u32 element_cnt,
        const void *data);
    void alloc_index(const std::vector<u8> &indices);
    void alloc_index(const std::vector<u16> &indices);
    void alloc_index(const std::vector<u32> &indices);

    void alloc_shader(const std::string &vertex_code,
        const std::string &fragment_code,
         const std::string &geometry_code = "", const std::string &tess_ctrl_code = "", const std::string &tess_eval_code = "");
    void alloc_constant(const std::string &name, u32 size, void *data);
    void alloc_render_target();
    void dealloc();
    bool inited();

    Handle get_handle(){
        /* retrieve the right 24 bits */
        return this->handle & 0xffffff;
    }
    RenderResourceType get_type(){
        return static_cast<RenderResourceType>(this->handle >> 24);
    }

    RenderResource() = default;
    ~RenderResource() = default;
};
}  // namespace Seed

#endif