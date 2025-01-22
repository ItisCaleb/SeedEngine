#ifndef _SEED_MODEL_ENTITY_H_
#define _SEED_MODEL_ENTITY_H_
#include "core/entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/rendering/material.h"

namespace Seed {
class ModelEntity : public Entity {
   private:
    Ref<Model> model;
    u32 mat_variant = 0;
   public:
    void update(f32 dt) override;
    void render(RenderCommandDispatcher &dp) override;
    void set_material_variant(u32 variant);
    Ref<Model> get_model();
    u32 get_material_variant();
    ModelEntity(Vec3 position, Ref<Model> model);
    ModelEntity(Ref<Model> model);

    ~ModelEntity() = default;
};

}  // namespace Seed

#endif