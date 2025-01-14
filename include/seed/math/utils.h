#ifndef _SEED_MATH_UTILS_H_
#define _SEED_MATH_UTILS_H_
#include <seed/types.h>

#define PI 3.14159265358979323846264338327
namespace Seed{
    f32 to_radians(f32 degree){
        return degree * PI / 180.0f;
    }

    f32 to_degree(f32 radians){
        return radians * 180.0f / PI;
    }
}

#endif