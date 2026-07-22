#ifndef _SEED_ANIMATION_H_
#define _SEED_ANIMATION_H_
#include <cmath>
#include <string>
#include "core/types.h"
#include "core/math/quaternion.h"
#include "core/math/vec3.h"
#include <vector>
#include "core/resource/resource.h"
#include "core/math/mat4.h"

namespace Seed {
struct PositionKey {
        f64 position_time;
        Vec3 position;
};

struct RotationKey {
        f64 rotation_time;
        Quaternion rotation;
};
struct ScalingKey {
        f64 scaling_time;
        Vec3 scaling;
};

struct AnimationClip {
        u16 bone_id;
        std::vector<PositionKey> position_keys;
        std::vector<RotationKey> rotation_keys;
        std::vector<ScalingKey> scaling_keys;
};

class ResourceLoader;
class Animation : public Resource {
        friend ResourceLoader;

    private:
        std::string name;
        f32 duration;
        /* clips for every bone */
        std::vector<AnimationClip> clips;

    public:
        std::vector<AnimationClip> &get_clips() { return clips; }
        f32 get_duration() const { return duration; }
        const std::string &get_name() const { return name; }
};

class AnimationState {
    private:
        Ref<Animation> animation;
        f32 current_time;
        f32 get_lerp_factor(f32 start_time, f32 end_time, f32 current_time) {
            f32 time_frame = end_time - start_time;
            f32 t = (current_time - start_time) / time_frame;
            return t;
        }
        Vec3 interpolate_position(AnimationClip &clip, f32 time);
        Quaternion interpolate_rotation(AnimationClip &clip, f32 time);
        Vec3 interpolate_scaling(AnimationClip &clip, f32 time);

    public:
        void set_animation(Ref<Animation> animation) {
            this->animation = animation;
            this->current_time = 0;
        }
        void update(f32 dt) {
            this->current_time += dt * 1000;
            if (this->animation.is_valid()) {
                this->current_time =
                    fmodf(this->current_time, this->animation->get_duration());
            }
        }
        void calculate_pose(Mat4 *bone_poses, u64 size);
};

}  // namespace Seed

#endif
