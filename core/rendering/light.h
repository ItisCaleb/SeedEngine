#ifndef _SEED_LIGHT_H_
#define _SEED_LIGHT_H_

#include "core/math/vec3.h"
#include "core/math/vec4.h"
#include "core/math/mat4.h"
#include "core/rendering/camera.h"

namespace Seed {


struct STB140Light {
        alignas(16) Vec3 position;
        alignas(16) Vec3 diffuse;
        alignas(16) Vec3 specular;
        f32 enable;
};


/*
layout(std140) uniform Lights {
    vec3 u_light_ambient;
    Light u_dir_light;
    Light u__point_lights[8];
};
*/
struct STB140Lights {
        alignas(16) Vec3 u_light_ambient;
        STB140Light u_dir_light;
        STB140Light u_point_lights[8];
};

enum class LightType : u8 { DIRECTIONAL, POINT, SPOT };

class Light {
    private:
        LightType type;
        bool dirty = true;
        Vec3 pos_dir;
        Vec3 diffuse;
        Vec3 specular;
        Mat4 light_mat;
        bool enable;

    public:
        inline STB140Light get_stb140() {
            return STB140Light{pos_dir, diffuse, specular, (f32)enable};
        }
        inline Mat4 get_light_space_mat() {
            if (dirty) return light_mat;
            Camera cam;
            if (type == LightType::DIRECTIONAL) {
                cam.set_ortho(-100, 100, -100, 100, -100, 100);
                // set position from origin
                cam.set_position(-pos_dir);
                cam.set_front(pos_dir);
                cam.set_up(Vec3{0, 1, 0});
                this->light_mat = cam.projection() * cam.look_at();
            }
            return light_mat;
        }
        Vec3 &get_position() {return pos_dir;}
        inline void set_enable(bool enable) { this->enable = enable; }
        Light(LightType type, const Vec3 &pos_dir, const Vec3 &diffuse,
              const Vec3 &specular, bool enable = true)
            : type(type),
              pos_dir(pos_dir),
              diffuse(diffuse),
              specular(specular),
              enable(enable) {}
};

}  // namespace Seed

#endif