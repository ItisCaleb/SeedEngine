#include "animation.h"
#include "core/rendering/light.h"

namespace Seed {

Vec3 AnimationState::interpolate_position(AnimationClip &clip, f32 time) {
    u32 i = 0;
    for (; i < clip.position_keys.size() - 1; i++) {
        if (time <= clip.position_keys[i].position_time) {
            PositionKey &p0 = clip.position_keys[i];
            PositionKey &p1 = clip.position_keys[i + 1];
            f32 t = get_lerp_factor(p0.position_time, p1.position_time, time);

            return Vec3::lerp(p0.position, p1.position, t);
        }
    }
    return clip.position_keys[i].position;
}
Quaternion AnimationState::interpolate_rotation(AnimationClip &clip, f32 time) {
    u32 i = 0;
    for (; i < clip.rotation_keys.size() - 1; i++) {
        if (time <= clip.rotation_keys[i].rotation_time) {
            RotationKey &r0 = clip.rotation_keys[i];
            RotationKey &r1 = clip.rotation_keys[i + 1];
            f32 t = get_lerp_factor(r0.rotation_time, r1.rotation_time, time);

            return Quaternion::nlerp(r0.rotation, r1.rotation, t);
        }
    }
    return clip.rotation_keys[i].rotation;
}
Vec3 AnimationState::interpolate_scaling(AnimationClip &clip, f32 time) {
    u32 i = 0;
    for (; i < clip.scaling_keys.size() - 1; i++) {
        if (time <= clip.scaling_keys[i].scaling_time) {
            ScalingKey &s0 = clip.scaling_keys[i];
            ScalingKey &s1 = clip.scaling_keys[i + 1];
            f32 t = get_lerp_factor(s0.scaling_time, s1.scaling_time, time);

            return Vec3::lerp(s0.scaling, s1.scaling, t);
        }
    }
    return clip.scaling_keys[0].scaling;
}
void AnimationState::calculate_pose(Mat4 *bone_poses, u64 size) {
    if (this->animation.is_null()) {
        SEED_WARN("Animation is null, skipping pose calculation.");
        return;
    }

    for (AnimationClip &clip : this->animation->get_clips()) {
        Mat4 translation =
            Mat4::translate_mat(interpolate_position(clip, current_time));
        Mat4 rotation =
            Mat4::rotate_mat(interpolate_rotation(clip, current_time));
        Mat4 scaling = Mat4::scale_mat(interpolate_scaling(clip, current_time));
        bone_poses[clip.bone_id] = translation * rotation * scaling;
    }
}

}  // namespace Seed