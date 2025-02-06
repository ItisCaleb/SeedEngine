#ifndef _SEED_GJK_H_
#define _SEED_GJK_H_
#include "core/math/vec3.h"

namespace Seed {
struct SimplexVertex {
    Vec3 p;
    /* we store vertex in origin polygon */
    Vec3 p1, p2;
    /* unnormalized barycentric coordinate for closest point */
    f32 u;
};

struct Simplex {
    struct SimplexVertex vers[4];
    struct SimplexVertex saved[4];
    u32 count;
    u32 last_count;

    void witness(Vec3 &p1, Vec3 &p2);
    Vec3 normal();
    Vec3 closest();
    void gjk_test();
};

}  // namespace Seed

#endif