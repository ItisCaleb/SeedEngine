#ifndef _SEED_MODEL_ENTITY_H_
#define _SEED_MODEL_ENTITY_H_
#include "core/entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/resource/material.h"

namespace Seed {
class ModelEntity : public Entity {
    private:
        Ref<Model> model;

    public:
        void update(f32 dt) override;
        void render() override;
        AABB get_model_aabb();
        Ref<Model> get_model();
        ModelEntity(Vec3 position, Ref<Model> model);
        ModelEntity(Ref<Model> model);

        ~ModelEntity() = default;
};

}  // namespace Seed

#endif