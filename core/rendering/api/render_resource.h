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
    VERTEX_DESC,
    INDEX,
    CONSTANT,
    SHADER,
    UNINITIALIZE
};
enum class VertexAttributeType: u8 { FLOAT, INT, UNSIGNED };

typedef u32 RenderResourceHandle;
struct VertexAttribute {
    u8 layout_num;
    VertexAttributeType type = VertexAttributeType::FLOAT;
    bool should_normalized = false;
    bool is_instance = false;
    u32 size;
    u32 stride;
};

struct RenderResource {
    RenderResourceHandle handle;
    RenderResourceType type = RenderResourceType::UNINITIALIZE;
    u16 stride = 0;
    union{
        u32 element_cnt = 0;
        u32 vertex_cnt;
    };

    void alloc_texture(u32 w, u32 h, const void *data);
    void alloc_vertex(u32 stride, u32 element_cnt,
                      void *data);
    void alloc_vertex_desc(std::vector<VertexAttribute> &attrs);
    void alloc_index(std::vector<u32> &indices);
    void alloc_shader(const std::string &vertex_code,
        const std::string &fragment_code,
         const std::string &geometry_code, const std::string &tess_ctrl_code, const std::string &tess_eval_code);
    void alloc_constant(const std::string &name, u32 size, void *data);
    void dealloc();
    bool inited();
    inline static std::map<std::string, RenderResource> texture;

    inline static u32 constant_cnt = 0;
    RenderResource() = default;
    ~RenderResource() = default;
};
}  // namespace Seed

#endif