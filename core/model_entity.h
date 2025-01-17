#ifndef _SEED_MODEL_ENTITY_H_
#define _SEED_MODEL_ENTITY_H_
#include "core/entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/rendering/material.h"

namespace Seed {
class ModelEntity : public Entity {
   private:
    RenderResource mesh_rc;
    RenderResource tex;
    RenderResource *matrices_rc;
    RenderResource *material_rc;
    Material material;

   public:
    void render(RenderCommandDispatcher &dp) override;
    void set_material(Material mat);
    void set_texture(RenderResource tex);
    ModelEntity(Vec3 position, Ref<Mesh> model);
    ModelEntity(Ref<Mesh> model);

    ~ModelEntity() = default;
};

}  // namespace Seed

#endif