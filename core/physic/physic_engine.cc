#include "physic_engine.h"
#include "jolt_backend.h"
#include <spdlog/spdlog.h>
#include "physic_shape.h"

namespace Seed {

static JoltBackend *backend;

PhysicEngine::PhysicEngine() { backend = new JoltBackend(); };

void PhysicEngine::create_body(PhysicBody &body, const PhysicShape &shape,
                               const PhysicBodyType type, const Vec3 &pos,
                               const Quaternion &quat) {
    if (shape.type == PhysicShapeType::EMPTY_SHAPE) {
        return;
    }
    backend->create_body(body, shape, type, pos, quat);
}
void PhysicEngine::delete_body(PhysicBody &body) {
    if (body.handle == NULL_HANDLE) {
        return;
    }
    backend->delete_body(body);
}

void PhysicEngine::process() { backend->process(); }

void PhysicEngine::query_physics(PhysicBody &body, Vec3 &position,
                                 Quaternion &quat) {
    backend->query_physics(body, position, quat);
}

void PhysicEngine::set_physics(PhysicBody &body, Vec3 &positon,
                               Quaternion &quat) {
    backend->set_physics(body, positon, quat);
}
}  // namespace Seed