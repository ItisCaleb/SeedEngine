#include "model.h"
#include "core/math/mat4.h"
 
namespace Seed {

Model::Model(const std::vector<Ref<Mesh>> &meshes,
              AABB bounding_box)
    : meshes(std::move(meshes)), bounding_box(bounding_box) {
        instance_rc.alloc_vertex(sizeof(Mat4), 1, NULL);
    }


AABB Model::get_bounding_box() { return bounding_box; }

Model::~Model() {
    instance_rc.dealloc();
}



}  // namespace Seed