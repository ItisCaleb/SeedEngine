#ifndef _SEED_LIGHT_H_
#define _SEED_LIGHT_H_

#include "core/math/vec3.h"
#include "core/math/vec4.h"

namespace Seed {


struct PosLight {
    alignas(16) Vec3 position;
    alignas(16) Vec3 ambient;
    alignas(16) Vec3 diffuse;
    alignas(16) Vec3 specular;
};

struct DirLight {
    alignas(16) Vec3 dir;
    alignas(16) Vec3 ambient;
    alignas(16) Vec3 diffuse;
    alignas(16) Vec3 specular;
};
}  // namespace Seed

#endif