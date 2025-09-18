#ifndef _SEED_SHAPE_H_
#define _SEED_SHAPE_H_
#include "core/types.h"
#include "core/math/vec3.h"  

namespace Seed {
struct Rect {
        u32 x, y, w, h;
};

struct RectF {
        f32 x, y, w, h;
};

struct Plane {
        Vec3 point;
        Vec3 normal;
};
}  // namespace Seed

#endif