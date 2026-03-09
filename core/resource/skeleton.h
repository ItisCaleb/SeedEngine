#ifndef _SEED_SKELETON_H_
#define _SEED_SKELETON_H_
#include "core/ref.h"
#include "core/math/mat4.h"
#include "core/rendering/instance_data.h"
#include <vector>
#include "core/resource/animation.h"

namespace Seed {
class ResourceLoader;

class Skeleton : public RefCounted {
    friend ResourceLoader;
    private:
        std::vector<Mat4> bones;
        /* we assure bone parents is topological order */
        std::vector<u16> bone_parents;

    public:
        void apply_fk(Mat4 *bone_tranforms, u64 size);
        void apply_skinning(Mat4 *bone_tranforms, u64 size);
        u64 size() { return bones.size(); }
};

class SkeletonInstanceData : public InstanceData {
    private:
        Ref<Skeleton> skeleton;
        struct SkeletonInstance {
                Ref<Transform> transform;
                Ref<AnimationState> state;
        };
        std::vector<SkeletonInstance> instances;

    public:
        const std::vector<SkeletonInstance> &get_instances() { return instances; }
        u32 size() override { return instances.size(); }
        void insert_instance(Ref<Transform> transform,
                             Ref<AnimationState> state);
        void remove_state(Ref<Transform> transform, Ref<AnimationState> state);
        void upload() override;
        void frustum_culling(const Frustum &frustum, const AABB &bounding_box,
                             std::vector<u32> &instance_ids,
                             std::vector<f32> &depths) override;

        SkeletonInstanceData(Ref<Skeleton> skeleton);
};
}  // namespace Seed

#endif