#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include "core/math/vec3.h"

namespace Seed {
struct Material {
    alignas(16) Vec3 ambient = {0.1, 0.1, 0.1};
    alignas(16) Vec3 diffuse = {0.7, 0.7, 0.7};
    alignas(16) Vec3 specular = {0.3, 0.3, 0.3};
    f32 shiness = 32;
};

}  // namespace Seed

#endif