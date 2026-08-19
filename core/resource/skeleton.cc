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

Handle SkeletonInstanceBatch::insert(const Transform &transform,
                                     AnimationState *state) {
    Instance inst = Instance{.world_matrix = transform.get_model_matrix(),
                             .state = AnimationState{}};
    if (state) {
        inst.state = *state;
    }
    Handle handle = this->instances.insert(inst);
    mark_dirty();
    return handle;
}

void SkeletonInstanceBatch::update(Handle handle, const Transform &transform,
                                   AnimationState *state) {
    if (handle == NULL_HANDLE || handle >= this->instances.size()) {
        return;
    }
    this->instances[handle].world_matrix = transform.get_model_matrix();
    if (state) {
        this->instances[handle].state = *state;
    } else {
        this->instances[handle].state = {};
    }
    mark_dirty();
}

void SkeletonInstanceBatch::remove(Handle handle) {
    this->instances.erase(handle);
    mark_dirty();
}

void SkeletonInstanceBatch::prepare_uploads(
    std::vector<RHI::UpdateBufferInfo> &uploads) {
    /* upload */
    for (Instance &inst : this->instances) {
        RHI::UpdateBufferInfo skeleton_info =
            RHI::alloc_heap(sizeof(Mat4) * (1 + skeleton->bone_count()));
        Mat4 *buffer = (Mat4 *)skeleton_info.data;
        buffer[0] = inst.world_matrix;
        inst.state.calculate_pose(&buffer[1], skeleton->bone_count());
        skeleton->apply_fk(&buffer[1], skeleton->bone_count());
        skeleton->apply_skinning(&buffer[1], skeleton->bone_count());
        uploads.push_back(skeleton_info);
    }
}

AABB SkeletonInstanceBatch::translate_bounding_box(const AABB &bounding_box,
                                                   u32 i) {
    Mat4 world_matrix = instances[i].world_matrix;
    AABB aabb = bounding_box.translate(world_matrix);
    return aabb;
}

SkeletonInstanceBatch::SkeletonInstanceBatch(Ref<Skeleton> skeleton)
    : skeleton(skeleton) {}
}  // namespace Seed
