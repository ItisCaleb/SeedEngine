#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_
#include "core/rendering/api/render_resource.h"
#include "core/collision/aabb.h"
#include "core/resource/resource.h"
#include "core/rendering/mesh.h"
#include <vector>

namespace Seed {

class RenderEngine;
class DefaultRenderer;

class Model : public Resource {
        friend RenderEngine;
        friend DefaultRenderer;

    private:
        std::vector<Ref<Mesh>> meshes;

    public:
        std::vector<Ref<Mesh>> &get_meshes(){
            return meshes;
        }
        Model(const std::vector<Ref<Mesh>> &meshes);
        ~Model();
};
}  // namespace Seed

#endif