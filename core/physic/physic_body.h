#ifndef _SEED_PHYSIC_BODY_H_
#define _SEED_PHYSIC_BODY_H_
#include "core/handle.h"
#include "core/ref.h"

namespace Seed {

enum class PhysicBodyType { STATIC, DYNAMIC, KINETIC };

struct PhysicBody {
        PhysicBodyType type;
        Handle handle = NULL_HANDLE;
};
}  // namespace Seed

#endif