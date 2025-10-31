#ifndef _SEED_OBJECT_H_
#define _SEED_OBJECT_H_
#include "core/math/vec3.h"
#include "core/math/mat4.h"
#include "core/rendering/mesh.h"
#include "core/rendering/api/render_command.h"
#include "core/physic/physic_body.h"
#include "core/physic/physic_shape.h"
#include "core/physic/physic_engine.h"
#include "core/resource/model.h"
#include "core/transform.h"

namespace Seed {
class Entity {
        friend PhysicEngine;

    protected:
        u32 id;
        Ref<Transform> transform;
        PhysicBody body;
        Ref<Model> model;

    public:
        Ref<Transform> get_transform() { return transform; }
        void create_body(PhysicShape &shape, PhysicBodyType type);
        void remove_body();
        void bind_model(Ref<Model> model);
        virtual void update(f32 dt) {}
        virtual void render() {}

        Entity(Vec3 position);
        Entity();
        ~Entity();
};

}  // namespace Seed

#endif