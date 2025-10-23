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
        Camera cam;
        bool enable;

    public:
        inline STB140Light get_stb140() {
            return STB140Light{pos_dir, diffuse, specular, (f32)enable};
        }
        Vec3 &get_position() { return pos_dir; }
        void set_dirty() { dirty = true; }
        inline void set_enable(bool enable) { this->enable = enable; }
        Light(LightType type, const Vec3 &pos_dir, const Vec3 &diffuse,
              const Vec3 &specular, bool enable = true)
            : type(type),
              pos_dir(pos_dir),
              diffuse(diffuse),
              specular(specular),
              enable(enable) {}
};

class DirectionalLight {

};

}  // namespace Seed

#endif