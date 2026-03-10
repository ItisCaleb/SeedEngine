#include "skeleton.h"
#include <spdlog/spdlog.h>
#include "core/rendering/rhi/render_engine.h"
#include "core/macro.h"

namespace Seed {
void Skeleton::apply_fk(Mat4 *bone_tranforms, u64 size) {
    if (size != bone_parents.size()) {
        SPDLOG_WARN("Bone pose array size doen't match bone size.");
        return;
    }
    for (u32 i = 0; i < size; i++) {
        u16 parent_id = this->bone_parents[i];
        if (parent_id == i) {
            bone_tranforms[i] = Mat4{};
        } else {
            bone_tranforms[i] = bone_tranforms[parent_id] * bone_tranforms[i];
        }
    }
}
void Skeleton::apply_skinning(Mat4 *bone_tranforms, u64 size) {
    for (u32 i = 0; i < size; i++) {
        bone_tranforms[i] = bone_tranforms[i] * bones[i];
    }
}

void SkeletonInstanceData::insert_instance(Ref<Transform> transform,
                                           Ref<AnimationState> state) {
    EXPECT_NOT_NULL_RET(*transform);
    EXPECT_NOT_NULL_RET(*state);

    this->instances.push_back(SkeletonInstance{transform, state});
}
void SkeletonInstanceData::remove_state(Ref<Transform> transform,
                                        Ref<AnimationState> state) {
    //this->instances.erase(SkeletonInstance{transform, state});
}
void SkeletonInstanceData::upload() {
    u64 size = this->instances.size() * (1 + skeleton->size());
    if (instance_handle == NULL_HANDLE) {
        instance_handle = pool->alloc(size);
    } else {
        auto block = pool->query(instance_handle);
        if (block.size < size) {
            pool->free(instance_handle);
            instance_handle = pool->alloc(size);
        }
    }
    /* upload */
    InstanceDataPool::Block block = pool->query(instance_handle);
    Mat4 *mats = (Mat4 *)RHI::alloc_heap(sizeof(Mat4) * size);
    u64 i = 0;
    for (const SkeletonInstance &instance : this->instances) {
        mats[i] = instance.transform->get_model_matrix();
        instance.state->calculate_pose(&mats[i + 1], skeleton->size());
        skeleton->apply_fk(&mats[i + 1], skeleton->size());
        skeleton->apply_skinning(&mats[i + 1], skeleton->size());
        i += 1 + skeleton->size();
    }
    RHI::update_from_heap(pool->get_render_buffer(), sizeof(Mat4) * block.idx,
                          sizeof(Mat4) * size, mats);
}
void SkeletonInstanceData::frustum_culling(const Frustum &frustum,
                                           const AABB &bounding_box,
                                           std::vector<u32> &instance_ids,
                                           std::vector<f32> &depths) {
    u32 i = pool->query(instance_handle).idx;

    for (const SkeletonInstance &instance : instances) {
        AABB aabb = instance.transform->translate_AABB(bounding_box);
        /* frustum culling */
        if (frustum.within_frustum(aabb)) {
            /* push instance indices */
            instance_ids.push_back(i);
            depths.push_back(
                frustum.calculate_depth(instance.transform->get_position()));
        }
        i++;
    }
}

SkeletonInstanceData::SkeletonInstanceData(Ref<Skeleton> skeleton)
    :InstanceData(
          RenderEngine::get_instance()->get_instance_pool(SKELETON_POOL_NAME)), skeleton(skeleton) {
}
}  // namespace Seed