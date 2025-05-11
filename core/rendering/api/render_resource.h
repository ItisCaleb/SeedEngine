#ifndef _SEED_RENDERING_RESOURCE_H_
#define _SEED_RENDERING_RESOURCE_H_
#include "core/types.h"
#include <vector>
#include <string>
#include <map>

namespace Seed {
enum class RenderResourceType: u16 {
    TEXTURE,
    VERTEX,
    INDEX,
    CONSTANT,
    SHADER,
    UNINITIALIZE
};

typedef u32 RenderResourceHandle;


struct RenderResource {
    RenderResourceHandle handle;
    RenderResourceType type = RenderResourceType::UNINITIALIZE;

    void alloc_texture(u32 w, u32 h, const void *data);
    void alloc_vertex(u32 stride, u32 element_cnt,
        const void *data);
    void alloc_index(const std::vector<u32> &indices);
    void alloc_shader(const std::string &vertex_code,
        const std::string &fragment_code,
         const std::string &geometry_code = "", const std::string &tess_ctrl_code = "", const std::string &tess_eval_code = "");
    void alloc_constant(const std::string &name, u32 size, void *data);
    void dealloc();
    bool inited();

    inline static u32 constant_cnt = 0;
    RenderResource() = default;
    ~RenderResource() = default;
};
}  // namespace Seed

#endif