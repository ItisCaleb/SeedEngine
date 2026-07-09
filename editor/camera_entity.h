#ifndef EDITOR_CAMERA_ENTITY
#define EDITOR_CAMERA_ENTITY
#include "core/types.h"
#include "core/world/behaviour.h"
#include "core/rendering/camera.h"
#include "core/world/entity.h"

namespace Seed {

class EditorCameraBehaviour : public Behaviour {
        Camera *cam;
        f32 yaw, pitch;
        f32 speed;

    public:
        virtual void start() override;
        virtual void update(f32 dt) override;
};

class EditorCameraEntity {
    public:
        static Entity create_entity(EntityManager &m);
        static void destroy_entity(EntityManager &m, Entity e);
};

}  // namespace Seed

#endif