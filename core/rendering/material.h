#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include "core/math/vec3.h"

namespace Seed {
struct Material {
    alignas(16)
    u32 texture;
    u32 diffuse;
    u32 specular;
    f32 shiness = 32;
};

}  // namespace Seed

#endif