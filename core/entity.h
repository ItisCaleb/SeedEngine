#ifndef _SEED_OBJECT_H_
#define _SEED_OBJECT_H_
#include "core/math/vec3.h"
#include "core/math/mat4.h"
#include "core/rendering/mesh.h"
#include "core/rendering/api/render_command.h"
#include "core/physic/physic_body.h"
#include "core/physic/physic_shape.h"
#include "core/physic/physic_engine.h"

namespace Seed {
class Entity {
    friend PhysicEngine;
    protected:
        u32 id;
        Vec3 position;
        Quaternion rotation;
        Vec3 scale;
        Mat4 transform;
        bool dirty = true;
        PhysicBody body;
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
        void create_body(PhysicShape &shape, PhysicBodyType type);
        void remove_body();
        virtual void update(f32 dt) {}
        virtual void render() {}

        Entity(Vec3 position);
        Entity();
        ~Entity() = default;
};

}  // namespace Seed

#endif