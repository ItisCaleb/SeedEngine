#include "shadow_map.h"
#include "core/macro.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace Seed {

bool ShadowMap::try_merge(u16 idx, u8 layer) {
    if (idx >= this->spaces.size()) return false;
    if (layer == 0) {
        return this->spaces[idx] == 0;
    }
    // top left
    if (!try_merge(idx, layer - 1)) return false;

    // top right
    if (!try_merge(idx + layer, layer - 1)) return false;

    // down left
    if (!try_merge(idx + width * layer, layer - 1)) return false;

    // down right
    if (!try_merge(idx + width * layer + layer, layer - 1)) return false;

    return true;
}

Handle ShadowMap::allocate_512() {
    for (u16 i = 0; i < this->spaces.size();) {
        if (try_merge(i, 0)) {
            this->spaces[i] = 1;
            return this->used_spaces.insert(i);
        }

        u8 next_jmp = std::max((u8)1, this->spaces[i]);
        i += next_jmp;
    }
    SPDLOG_DEBUG("Unable to allocate a 512 * 512 shadowmap");
    return NULL_HANDLE;
}
Handle ShadowMap::allocate_1024() {
    for (u16 i = 0; i < this->spaces.size();) {
        if (try_merge(i, 1)) {
            for (u16 j = 0; j < 2; j++) {
                for (u16 k = 0; k < 2; k++) {
                    this->spaces[i + j * width + k] = 2;
                }
            }
            return this->used_spaces.insert(i);
        }

        u8 next_jmp = std::max((u8)2, this->spaces[i]);
        if (i % width > (i + next_jmp) % width) {
            next_jmp += width;
        }
        i += next_jmp;
    }
    SPDLOG_DEBUG("Unable to allocate a 1024 * 1024 shadowmap");
    return NULL_HANDLE;
}
Handle ShadowMap::allocate_2048() {
    for (u16 i = 0; i < this->spaces.size();) {
        if (try_merge(i, 2)) {
            for (u16 j = 0; j < 4; j++) {
                for (u16 k = 0; k < 4; k++) {
                    this->spaces[i + j * width + k] = 4;
                }
            }
            return this->used_spaces.insert(i);
        }

        u8 next_jmp = std::max((u8)4, this->spaces[i]);
        if (i % width > (i + next_jmp) % width) {
            next_jmp += width * 3;
        }
        i += next_jmp;
    }
    SPDLOG_DEBUG("Unable to allocate a 2048 * 2048 shadowmap");
    return NULL_HANDLE;
}

Viewport ShadowMap::query_viewport(Handle handle) {
    u16 *idx = this->used_spaces.get_or_null(handle);
    if (!idx) {
        SPDLOG_ERROR("Illegal handle in shadowmap.");
        return Viewport(Vec2{(f32)resolution, (f32)resolution});
    }
    f32 unit = this->min_res / (f32)resolution;
    f32 res = unit * this->spaces[*idx];
    f32 x = unit * (*idx % width);
    f32 y = unit * (*idx / width);
    return Viewport(RectF{x, y, res, res},
                    Vec2{(f32)resolution, (f32)resolution});
}

RectF ShadowMap::query_uv(Handle handle) {
    u16 *idx = this->used_spaces.get_or_null(handle);
    if (!idx) return RectF{};
    f32 unit = (f32)min_res / (f32)resolution;
    f32 x = unit * (*idx % width);
    f32 res = unit * this->spaces[*idx];
    f32 y = 1.0 - unit * (*idx / width) - res;
    return RectF{x, y, res, res};
}

ShadowMap::ShadowMap() {
    this->shadow_map.create(TextureType::TEXTURE_2D, resolution, resolution,
                            PixelFormat::D32, nullptr);
    this->width = resolution / min_res;
    this->spaces.resize(width * width);
}
ShadowMap::~ShadowMap() {}
}  // namespace Seed
