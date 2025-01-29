#ifndef _SEED_AABB_H_
#define _SEED_AABB_H_
#include "core/types.h"
#include "core/math/mat4.h"

namespace Seed {
struct AABB {
    Vec3 center;
    Vec3 ext;
    bool collide(AABB &other) {
        return center.x + ext.x >= other.center.x - other.ext.x &&
               center.x - ext.x <= other.center.x + other.ext.x &&
               center.y + ext.y >= other.center.y - other.ext.y &&
               center.y - ext.y <= other.center.y + other.ext.y &&
               center.z + ext.z >= other.center.z - other.ext.z &&
               center.z - ext.z <= other.center.z + other.ext.z;
    }

};

}  // namespace Seed
#endif