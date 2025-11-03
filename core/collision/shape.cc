#include "shape.h"
namespace Seed {
Vec3 plane_intersect(const Plane &p1, const Plane &p2, const Plane &p3) {
    Vec3 n1 = p1.normal;
    Vec3 n2 = p2.normal;
    Vec3 n3 = p3.normal;

    f32 d1 = -n1.dot(p1.point);
    f32 d2 = -n2.dot(p2.point);
    f32 d3 = -n3.dot(p3.point);

    Vec3 n2n3 = n2.cross(n3);
    Vec3 n3n1 = n3.cross(n1);
    Vec3 n1n2 = n1.cross(n2);

    f32 denom = n1.dot(n2n3);
    return (-n2n3 * d1 - n3n1 * d2 - n1n2 * d3) / denom;
}
}  // namespace Seed