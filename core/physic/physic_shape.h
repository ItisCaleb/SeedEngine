#ifndef _SEED_PHYSIC_SHAPE_H_
#define _SEED_PHYSIC_SHAPE_H_
#include "core/types.h"
#include "core/handle.h"
#include "core/math/vec3.h"
#include <vector>

namespace Seed {
enum class PhysicShapeType : u8 { SPHERE, BOX, CAPSULE, PLANE, HEIGHT_MAP };

class PhysicShape {
    public:
        const PhysicShapeType type;

    protected:
        PhysicShape(PhysicShapeType type) : type(type) {}
};

class PhysicBoxShape : public PhysicShape {
    public:
        Vec3 half_extent;
        PhysicBoxShape(Vec3 half_extent)
            : PhysicShape(PhysicShapeType::BOX), half_extent(half_extent) {}
};

class PhysicHeightmapShape : public PhysicShape {
    public:
        PhysicHeightmapShape();
};

class PhysicCylinderShape : public PhysicShape {
    public:
        PhysicCylinderShape();
};

}  // namespace Seed

#endif