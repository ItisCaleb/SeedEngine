#ifndef _SEED_OBJECT_H_
#define _SEED_OBJECT_H_
#include "core/math/vec3.h"
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
        Ref<SkeletonModel> skeleton_model;
        Ref<AnimationState> state;
    public:
        Ref<Transform> get_transform() { return transform; }
        void create_body(PhysicShape &shape, PhysicBodyType type);
        void remove_body();
        void bind_model(Ref<Model> model);
        void bind_skeleton_model(Ref<SkeletonModel> model);
        virtual void update(f32 dt);
        void play_animation(const std::string &name);
        Entity(Vec3 position);
        Entity();
        ~Entity();
};

}  // namespace Seed

#endif