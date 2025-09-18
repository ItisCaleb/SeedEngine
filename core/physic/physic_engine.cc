#include "physic_engine.h"
#include "jolt_backend.h"
#include <spdlog/spdlog.h>
#include "core/engine.h"


namespace Seed {

static JoltBackend *backend;

PhysicEngine::PhysicEngine() {
    instance = this;
    backend = new JoltBackend();
};

void PhysicEngine::create_body(PhysicBody &body, PhysicShape &shape,
                               PhysicBodyType type, Vec3 &pos,
                               const Quaternion &quat) {
    backend->create_body(body, shape, type, pos, quat);
}
void PhysicEngine::delete_body(PhysicBody &body) { backend->delete_body(body); }

void PhysicEngine::process() { backend->process();
    auto entities = SeedEngine::get_instance()->get_world()->get_entities();
    for (auto entity: entities){
        if(entity->body.handle != NULL_HANDLE){
            backend->query_physics(entity->body, entity->position, entity->rotation);

            entity->dirty = true;
        }
    }
}
}  // namespace Seed