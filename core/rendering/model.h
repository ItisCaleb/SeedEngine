#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_
#include "core/ref.h"
#include "api/render_resource.h"
#include "core/collision/aabb.h"
#include "mesh.h"
#include <vector>

namespace Seed {

class RenderEngine;
class ColorPass;


class Model : public RefCounted {
    friend RenderEngine;
    friend ColorPass;
   private:
    RenderResource instance_rc;
    std::vector<Ref<Material>> materials;
    std::vector<Mesh> meshes;
    AABB bounding_box;
   public:
    Model(const std::vector<Mesh> &meshes,
          const std::vector<Ref<Material>> &mats, AABB bounding_box);
    Ref<Material> get_material(u32 material_handle);
    AABB get_bounding_box();
    ~Model();
};
}  // namespace Seed

#endif