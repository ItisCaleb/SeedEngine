#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_
#include "core/ref.h"
#include "api/render_resource.h"
#include "core/collision/aabb.h"
#include "mesh.h"
#include <vector>

namespace Seed {

class RenderEngine;
class ModelRenderer;


class Model : public RefCounted {
    friend RenderEngine;
    friend ModelRenderer;
   private:
    RenderResource instance_rc;
    std::vector<Mesh> meshes;
    AABB bounding_box;
   public:
    Model(const std::vector<Mesh> &meshes, AABB bounding_box);
    AABB get_bounding_box();
    ~Model();
};
}  // namespace Seed

#endif