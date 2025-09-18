#ifndef _SEED_PHYSIC_ENGINE_H_
#define _SEED_PHYSIC_ENGINE_H_
#include "core/physic/physic_shape.h"
#include "core/physic/physic_body.h"
#include "core/math/vec3.h"
#include "core/math/quaternion.h"
namespace Seed {

class PhysicEngine {
    private:
        inline static PhysicEngine *instance = nullptr;

    public:
        static PhysicEngine *get_instance() { return instance; }
        void process();
        void create_body(PhysicBody &body, PhysicShape &shape,
                         PhysicBodyType type, Vec3 &pos,
                         const Quaternion &quat = Quaternion::identity());
        void delete_body(PhysicBody &body);
        PhysicEngine();
};
}  // namespace Seed

#endif