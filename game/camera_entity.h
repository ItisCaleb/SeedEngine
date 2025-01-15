#ifndef CAMERA_ENTITY
#define CAMERA_ENTITY
#include "core/entity.h"


namespace Seed
{
    class CameraEntity: public Entity
    {
    private:
        /* data */
    public:
        void update(f32 dt) override;
        CameraEntity() = default;
        ~CameraEntity();
    };



} // namespace Seed



#endif