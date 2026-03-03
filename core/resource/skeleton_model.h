#ifndef _SEED_SKELETON_MODEL_H_
#define _SEED_SKELETON_MODEL_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/math/vec4.h"
#include "core/resource/resource.h"
#include "core/rendering/mesh.h"
#include "core/rendering/instance_data.h"
#include <vector>

namespace Seed {

class RenderEngine;
class DefaultRenderer;



class SkeletonInstanceData : public InstanceData {
    private:
        std::set<Ref<Transform>> transforms;

    public:
        const std::set<Ref<Transform>> &get_transforms() { return transforms; }
        u32 get_size() override { return transforms.size(); }
        void insert_transform(Ref<Transform> transform);
        void remove_transform(Ref<Transform> transform);
        void upload() override;
        void frustum_culling(const Frustum &frustum, const AABB &bounding_box,
                             std::vector<u32> &instance_ids,
                             std::vector<f32> &depths) override;

        SkeletonInstanceData();
};

class SkeletonModel : public Resource {
        friend RenderEngine;
        friend DefaultRenderer;

    private:
        std::vector<Ref<Mesh>> meshes;
        // Ref<TransformInstanceData> instances;

    public:
        std::vector<Ref<Mesh>> &get_meshes() { return meshes; }
        SkeletonModel(const std::vector<Ref<Mesh>> &meshes);
        // void insert_transform(Ref<Transform> transform);
        // void remove_transform(Ref<Transform> transform);
        ~SkeletonModel();
};
}  // namespace Seed

#endif