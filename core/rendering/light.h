#ifndef _SEED_LIGHT_H_
#define _SEED_LIGHT_H_

#include "core/math/vec3.h"
#include "core/math/mat4.h"
#include "core/rendering/camera.h"
#include "core/macro.h"

namespace Seed {

struct CSMShadow {
        RectF shadow_uv[4];
        f32 fars[4];
        f32 units[4];
};

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
    protected:
        LightType type;
        Vec3 diffuse;
        Vec3 specular;
        bool enable;
        Light(LightType type, const Vec3 &diffuse, const Vec3 &specular,
              bool enable = true)
            : type(type),
              diffuse(diffuse),
              specular(specular),
              enable(enable) {}

    public:
        virtual void get_stb140(STB140Light *light) = 0;
};

class DirectionalLight : public Light {
    private:
        Vec3 dir;
        f32 shadow_lamdba = 0.8;
        Frustum frustum_cache[4];

    public:
        void get_stb140(STB140Light *light) override {
            light->position = dir;
            light->diffuse = diffuse;
            light->specular = specular;
            light->enable = enable;
        }

        inline Vec3 get_direction() { return dir; }
        inline void set_direction(const Vec3 &dir) { this->dir = dir; }
        void calculate_csm_lightspace(Camera &cam,
                                      const std::vector<f32> &resolutions,
                                      CSMShadow &csm_data,
                                      Camera::ShaderCamera *light_space_cam);

        void set_csm_lamda(f32 lamda) { this->shadow_lamdba = lamda; }
        f32 get_csm_lamda() { return shadow_lamdba; }

        const Frustum &get_frustum(u32 split) {
            EXPECT_INDEX_INBOUND_THROW(split, 4);
            return this->frustum_cache[split];
        }

        DirectionalLight(const Vec3 &dir, const Vec3 &diffuse,
                         const Vec3 &specular, bool enable = true)
            : Light(LightType::DIRECTIONAL, diffuse, specular, enable),
              dir(dir) {}
};

class PointLight : public Light {
    private:
        Vec3 pos;
        bool dirty = true;
        Mat4 light_space_proj;
        Mat4 light_space_lookat[6];
        Frustum frustum_cache[6];

    public:
        void get_stb140(STB140Light *light) override {
            light->position = pos;
            light->diffuse = diffuse;
            light->specular = specular;
            light->enable = enable;
        }

        void calculate_lightspace(Camera::ShaderCamera *light_space_cam);
        const Frustum &get_frustum(u32 index) {
            EXPECT_INDEX_INBOUND_THROW(index, 6);
            return this->frustum_cache[index];
        }
        inline Vec3 get_position() { return pos; }

        PointLight(const Vec3 &pos, const Vec3 &diffuse, const Vec3 &specular,
                   bool enable = true)
            : Light(LightType::POINT, diffuse, specular, enable), pos(pos) {}
};

}  // namespace Seed

#endif