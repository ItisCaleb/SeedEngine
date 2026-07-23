#include "skeleton.h"
#include <spdlog/spdlog.h>
#include "core/math/mat4.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/macro.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/animation.h"
#include "core/system.h"
#include "core/transform.h"

namespace Seed {
void Skeleton::apply_fk(Mat4 *bone_tranforms, u64 size) {
    if (size != bone_parents.size()) {
        SEED_WARN(
            "Bone pose array size doen't match bone size. Skipping apply fk.");
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

void SkeletonInstanceBatch::insert_instance(Transform &transform,
                                           AnimationState *state) {
    RHI::UpdateBufferInfo skeleton_info =
        RHI::alloc_heap(sizeof(Mat4) * (1 + skeleton->bone_count()));
    Mat4 *buffer = (Mat4 *)skeleton_info.data;
    buffer[0] = transform.get_model_matrix();
    if (state) {
        state->calculate_pose(&buffer[1], skeleton->bone_count());
    } else {
        for (u32 i = 0; i < skeleton->bone_count(); i++) {
            buffer[1 + i] = Mat4{};
        }
    }
    this->upload_buffers.push_back(skeleton_info);
}

void SkeletonInstanceBatch::upload() {
    /* upload */
    for (RHI::UpdateBufferInfo &info : this->upload_buffers) {
        Mat4 *buffer = (Mat4 *)info.data;
        skeleton->apply_fk(&buffer[1], skeleton->bone_count());
        skeleton->apply_skinning(&buffer[1], skeleton->bone_count());
    }
    _upload(this->upload_buffers);
}
void SkeletonInstanceBatch::frustum_culling(const Frustum &frustum,
                                           const AABB &bounding_box,
                                           std::vector<u32> &instance_ids,
                                           std::vector<f32> &depths) {
    u32 i = pool->query(instance_handle).idx;

    for (const RHI::UpdateBufferInfo &info : upload_buffers) {
        Mat4 world_matrix = ((Mat4 *)info.data)[0];
        AABB aabb = bounding_box.translate(world_matrix);
        /* frustum culling */
        if (frustum.within_frustum(aabb)) {
            /* push instance indices */
            instance_ids.push_back(i);
            depths.push_back(frustum.calculate_depth(aabb.center));
        }
        i += instance_size();
    }
}

SkeletonInstanceBatch::SkeletonInstanceBatch(Ref<Skeleton> skeleton)
    : InstanceBatch(
          System::gRenderEngine->get_instance_pool(SKELETON_POOL_NAME)),
      skeleton(skeleton) {}
}  // namespace Seed
