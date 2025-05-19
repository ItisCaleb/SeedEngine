#ifndef _SEED_LIGHT_H_
#define _SEED_LIGHT_H_

#include "core/math/vec3.h"
#include "core/math/vec4.h"

namespace Seed {

struct Light {
        /* 0 off */
        /* -1 position*/
        /* -2 direction*/
        /* 1 - 180 spotlight*/
        alignas(16) Vec4 position;
        alignas(16) Vec3 diffuse;
        alignas(16) Vec3 specular;

        void set_position(Vec3 pos) {
            this->position = Vec4{pos.x, pos.y, pos.z, -1};
        }
        void set_direction(Vec3 dir) {
            this->position = Vec4{dir.x, dir.y, dir.z, -2};
        }
        void set_spotlight(Vec3 pos, f32 cutoff_angle) {
            cutoff_angle = cutoff_angle < 1     ? 1
                           : cutoff_angle > 180 ? 180
                                                : cutoff_angle;
            this->position = Vec4{pos.x, pos.y, pos.z, cutoff_angle};
        }
};

struct Lights {
        alignas(16) Vec3 ambient;
        Light lights[4];
};
}  // namespace Seed

#endif