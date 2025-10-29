#ifndef _SEED_SHAPE_H_
#define _SEED_SHAPE_H_
#include "core/types.h"
#include "core/math/vec3.h"

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

struct Rect {
        u32 x, y, w, h;
};

struct RectF {
        f32 x, y, w, h;
};

struct Plane {
        Vec3 point;
        Vec3 normal;
        bool within_plane(const AABB &aabb) const {
            // Compute the projection interval radius of b onto L(t) = b.c + t *
            // p.n
            float r = aabb.ext.x * std::abs(normal.x) +
                      aabb.ext.y * std::abs(normal.y) +
                      aabb.ext.z * std::abs(normal.z);

            // Compute distance of box center from plane
            // (n . C) - d = (n . C) - (n . P) = n . (C - P)
            float s = normal.dot(aabb.center - point);

            // Intersection occurs when distance s falls within [-r,+r] interval
            return -r <= s;
        }
};

struct Frustum {
        Plane left, right;
        Plane top, bottom;
        Plane near, far;
        bool within_frustum(const AABB &aabb) const{
            return right.within_plane(aabb) && left.within_plane(aabb) &&
                   top.within_plane(aabb) && bottom.within_plane(aabb) &&
                   near.within_plane(aabb) && far.within_plane(aabb);
        }
        f32 calculate_depth(const Vec3 &pos)const {
            float dist = (pos - this->near.point).length();
            return dist / (far.point - near.point).length();
        }
};

}  // namespace Seed

#endif