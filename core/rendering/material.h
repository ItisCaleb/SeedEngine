#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include "core/math/vec3.h"

namespace Seed{
    struct Material
    {
        Vec3 ambient;
        Vec3 diffuse;
        Vec3 specular;
        f32 shiness;
    };
    
}


#endif