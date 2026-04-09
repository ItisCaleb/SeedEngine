#ifndef HUMAN_ENTITY
#define HUMAN_ENTITY
#include "core/resource/animation.h"
#include "core/resource/model.h"
#include "core/types.h"
#include "core/world/behaviour.h"
#include "core/world/entity.h"

namespace Seed {

class HumanBehaviour : public Behaviour {
    private:
        Ref<SkeletonModel> model;
        AnimationState *state;
    public:
        HumanBehaviour();
        virtual void start() override;
        virtual void update(float dt) override;
};

class HumanEntity {
    public:
        static Entity create_entity(EntityManager &m, Transform &transform,
                                    Ref<SkeletonModel> model);
        static void destroy_entity(EntityManager &m, Entity e);
};

}  // namespace Seed

#endif