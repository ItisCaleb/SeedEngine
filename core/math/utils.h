#ifndef _SEED_MATH_UTILS_H_
#define _SEED_MATH_UTILS_H_
#include "core/types.h"

#define PI 3.14159265358979323846264338327
namespace Seed {
inline static f32 radians(f32 degree) { return degree * PI / 180.0f; }

inline static f32 degree(f32 radians) { return radians * 180.0f / PI; }

inline static u32 roundup_to_pow2(u32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

inline static u64 roundup_to_pow2(u64 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

}  // namespace Seed

#endif