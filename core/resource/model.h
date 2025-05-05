#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_
#include "core/rendering/api/render_resource.h"
#include "core/collision/aabb.h"
#include "core/resource/resource.h"
#include "core/rendering/mesh.h"
#include <vector>

namespace Seed {

class RenderEngine;
class ModelRenderer;


class Model : public Resource {
    friend RenderEngine;
    friend ModelRenderer;
   private:
    RenderResource instance_rc;
    std::vector<Ref<Mesh>> meshes;
    AABB bounding_box;
   public:
    Model(const std::vector<Ref<Mesh>> &meshes, AABB bounding_box);
    AABB get_bounding_box();
    ~Model();
};
}  // namespace Seed

#endif