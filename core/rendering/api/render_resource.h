#ifndef _SEED_RENDERING_RESOURCE_H_
#define _SEED_RENDERING_RESOURCE_H_
#include "core/types.h"
#include <vector>
#include <string>
#include <map>

namespace Seed {
enum class RenderResourceType { TEXTURE, VERTEX, CONSTANT, SHADER, UNINITIALIZE };

typedef u32 RenderResourceHandle;


struct RenderResource {
    RenderResourceType type = RenderResourceType::UNINITIALIZE;
    RenderResourceHandle handle;
    u32 element_cnt = 0;

    void alloc_texture(u32 w, u32 h, void* data);
    void alloc_vertex(u32 stride, std::vector<u32> &lens, u32 element_cnt, void *data);
    void alloc_shader(const char *vertex_code,
                                  const char *fragment_code);
    void alloc_constant(u32 size, void *data);
    void dealloc();

    inline static std::map<std::string, RenderResource> texture;
    inline static std::map<std::string, RenderResource> constants;
    inline static std::map<std::string, RenderResource> shaders;
    inline static u32 constant_cnt = 0;
    static void register_resource(const std::string &name, RenderResource rc);
    RenderResource() = default;
    ~RenderResource() = default;
};
}  // namespace Seed

#endif