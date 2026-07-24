#ifndef _SEED_SKELETON_H_
#define _SEED_SKELETON_H_
#include "core/ref.h"
#include "core/math/mat4.h"
#include "core/rendering/instance_batch.h"
#include <vector>

namespace Seed {
class AnimationState;
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
        u64 bone_count() { return bones.size(); }
};

class SkeletonInstanceBatch : public InstanceBase<SkeletonInstanceBatch> {
    private:
        Ref<Skeleton> skeleton;
        std::vector<RHI::UpdateBufferInfo> upload_buffers;

    public:
        u32 size() override { return upload_buffers.size(); }
        void insert_instance(Transform &transform, AnimationState *state);
        void prepare_uploads(
            std::vector<RHI::UpdateBufferInfo> &uploads) override;
        AABB translate_bounding_box(const AABB &bounding_box, u32 i) override;
        virtual u32 element_per_instance() override {
            return 1 + skeleton->bone_count();
        }
        void clear() override { upload_buffers.clear(); }
        SkeletonInstanceBatch(Ref<Skeleton> skeleton);
};
}  // namespace Seed

#endif
