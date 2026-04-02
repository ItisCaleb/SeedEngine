#ifndef _SEED_BEHAVIOUR_H_
#define _SEED_BEHAVIOUR_H_

#include "core/ref.h"
#include "core/world/entity.h"
namespace Seed {
class World;
class Behaviour : public RefCounted {
    friend World;
    protected:
        EntityManager *m;
        Entity self;

    public:
        virtual void start() {};
        virtual void update(float dt) {};
};

struct BehaviourComponent {
        Ref<Behaviour> behaviour;
};
}  // namespace Seed

#endif