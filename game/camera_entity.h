#ifndef CAMERA_ENTITY
#define CAMERA_ENTITY
#include "core/entity.h"
#include "core/rendering/rhi/render_engine.h"

namespace Seed {
class CameraEntity : public Entity {
    private:
        Camera *cam;
        f32 yaw = 0, pitch = 0;

    public:
        void update(f32 dt) override;
        CameraEntity();
        ~CameraEntity();
};

}  // namespace Seed

#endif