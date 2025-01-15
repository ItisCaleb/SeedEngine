#ifndef _SEED_LIGHT_H_
#define _SEED_LIGHT_H_

#include <seed/math/vec3.h>

namespace Seed{
    struct Light
    {
        Vec3 ambient;
        Vec3 diffuse;
        Vec3 specular;
    };
    
}


#endif