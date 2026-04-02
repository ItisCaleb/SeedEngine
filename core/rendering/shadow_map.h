#ifndef _SEED_SHADOW_MAP_H_
#define _SEED_SHADOW_MAP_H_
#include "core/handle.h"
#include "core/collision/shape.h"
#include "core/resource/texture.h"
#include "core/rendering/viewport.h"
#include <vector>

namespace Seed {
class ShadowMap {
        /*
        ----------------------
        | 4 | 4 | 4 | 4 | 1 | 1 | 2 | 2 |
        | 4 | 4 | 4 | 4 | 0 | 0 | 2 | 2 |
        | 4 | 4 | 4 | 4 | 2 | 2 |
        | 4 | 4 | 4 | 4 | 2 | 2 |
        */
    private:
        HandleOwner<u16> used_spaces;
        const static u32 min_res = 512;
        Ref<Texture> shadow_map;
        u32 resolution = 4096;
        u16 width;
        std::vector<u8> spaces;
        bool try_merge(u16 idx, u8 layer);

    public:
        Handle allocate_512();
        Handle allocate_1024();
        Handle allocate_2048();
        Viewport query_viewport(Handle handle);
        RectF query_uv(Handle handle);
        Ref<Texture> get_texture() { return shadow_map; }
        u32 get_resolution() { return resolution; }

        ShadowMap();
        ~ShadowMap();
};

}  // namespace Seed

#endif