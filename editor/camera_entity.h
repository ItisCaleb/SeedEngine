#ifndef EDITOR_CAMERA_ENTITY
#define EDITOR_CAMERA_ENTITY
#include "core/types.h"
#include "core/math/vec3.h"
#include "core/world/behaviour.h"
#include "core/world/entity.h"

namespace Seed {

class Camera;

class EditorCameraBehaviour : public Behaviour {
        enum class NavigationMode { None, Fly, Pan, Orbit };

        Camera *cam = nullptr;
        NavigationMode navigation_mode = NavigationMode::None;
        Vec3 orbit_center;
        f32 orbit_distance = 500.0f;
        f32 yaw = 0.0f;
        f32 pitch = 0.0f;
        f32 speed = 1000.0f;
        i32 last_mouse_x = 0;
        i32 last_mouse_y = 0;

        bool begin_navigation(NavigationMode mode);
        void end_navigation();
        void rotate(f32 delta_x, f32 delta_y);
        void focus(Vec3 target);

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
