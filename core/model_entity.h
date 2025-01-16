#ifndef _SEED_MODEL_ENTITY_H_
#define _SEED_MODEL_ENTITY_H_
#include "core/entity.h"
#include "core/rendering/api/render_engine.h"


namespace Seed
{
    class ModelEntity: public Entity
    {
    private:
        RenderResource mesh_rc;
        RenderResource *matrices_rc;
    public:
        void render(RenderCommandDispatcher &dp) override;
        ModelEntity(Vec3 position, Ref<Mesh> model);
        ModelEntity(Ref<Mesh> model);

        ~ModelEntity() = default;
    };



} // namespace Seed



#endif