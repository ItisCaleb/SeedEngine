#ifndef CAMERA_ENTITY
#define CAMERA_ENTITY
#include "core/types.h"
#include "core/world/behaviour.h"
#include "core/world/entity.h"

namespace Seed {

class Camera;

class CameraBehaviour : public Behaviour {
        Camera *cam;
        f32 yaw, pitch;

    public:
        virtual void start() override;
        virtual void update(float dt) override;
};

class CameraEntity {
    public:
        static Entity create_entity(EntityManager &m);
        static void destroy_entity(EntityManager &m, Entity e);
};

}  // namespace Seed

#endif
