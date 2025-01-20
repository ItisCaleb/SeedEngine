#ifndef _SEED_MODEL_ENTITY_H_
#define _SEED_MODEL_ENTITY_H_
#include "core/entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/rendering/material.h"

namespace Seed {
class ModelEntity : public Entity {
   private:
    Ref<Mesh> mesh;
    Ref<Material> mat;
   public:
    void update(f32 dt) override;
    void render(RenderCommandDispatcher &dp) override;
    void set_material(Ref<Material> mat);
    Ref<Mesh> get_mesh();
    Ref<Material> get_material();

    ModelEntity(Vec3 position, Ref<Mesh> model);
    ModelEntity(Ref<Mesh> model);

    ~ModelEntity() = default;
};

}  // namespace Seed

#endif