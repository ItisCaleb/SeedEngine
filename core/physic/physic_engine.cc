#include "physic_engine.h"
#include "jolt_backend.h"
#include <spdlog/spdlog.h>

namespace Seed {

static JoltBackend* backend;

PhysicEngine::PhysicEngine() {
    instance = this;
    backend = new JoltBackend();

};

void PhysicEngine::process() {
    backend->process();
}
}  // namespace Seed