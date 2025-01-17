#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include "core/math/vec3.h"



namespace Seed{
    struct Material
    {
        alignas(16) Vec3 ambient;
        alignas(16) Vec3 diffuse;
        alignas(16) Vec3 specular;
        f32 shiness;
    };

}


#endif