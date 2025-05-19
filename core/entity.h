#ifndef _SEED_OBJECT_H_
#define _SEED_OBJECT_H_
#include "math/vec3.h"
#include "math/mat4.h"
#include "rendering//mesh.h"
#include "rendering/api/render_command.h"

namespace Seed {
class Entity {
    protected:
        u32 id;
        Vec3 position;
        Vec3 rotation;
        Vec3 scale;
        Mat4 transform;
        bool dirty = true;
        void update_transform();

    public:
        Vec3 get_position();
        void set_position(Vec3 position);
        Vec3 get_rotation();
        void set_rotation(Vec3 rotation);
        void rotate(f32 x_angle, f32 y_angle, f32 z_angle);
        Vec3 get_scale();
        void set_scale(Vec3 scale);
        Mat4 get_transform();
        virtual void update(f32 dt) {}
        virtual void render() {}

        Entity(Vec3 position);
        Entity();
        ~Entity() = default;
};

}  // namespace Seed

#endif