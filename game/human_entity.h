#ifndef _SEED_HUMAN_ENTITY_H_
#define _SEED_HUMAN_ENTITY_H_
#include "core/rendering/camera.h"

#include "core/entity.h"
namespace Seed {
class HumanEntity : public Entity {
    private:
        Camera *cam;

    public:
        HumanEntity(Vec3 position);
        void update(f32 dt);
        ~HumanEntity() = default;
};
}  // namespace Seed

#endif