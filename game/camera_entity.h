#ifndef CAMERA_ENTITY
#define CAMERA_ENTITY
#include "core/entity.h"
#include "core/rendering/api/render_engine.h"

namespace Seed {
class CameraEntity : public Entity {
   private:
    Camera *cam;

   public:
    void update(f32 dt) override;
    void render(RenderCommandDispatcher &dp) override;
    CameraEntity();
    ~CameraEntity();
};

}  // namespace Seed

#endif