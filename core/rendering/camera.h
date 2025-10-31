#ifndef _SEED_CAMERA_H_
#define _SEED_CAMERA_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/math/mat4.h"
#include "core/collision/shape.h"
#include "core/collision/shape.h"
#include <vector>

namespace Seed {
class DirectionalLight;
class Camera {
        friend DirectionalLight;

    private:
        Vec3 position;
        Vec3 up;
        Vec3 front;
        struct {
                bool is_ortho;
                f32 left, right;
                f32 bottom, top;
                f32 near, far;
        } frustum;
        Frustum frustum_plane;
        Mat4 lookat_mat;
        bool dirty = true;
        void calculate_frustum();
        void calculate_lookat();
        void calculate_dirty();

    public:
        void set_position(Vec3 pos);
        Vec3 get_position();
        void set_up(Vec3 up);
        Vec3 get_up();
        void set_front(Vec3 front);
        void set_front(f32 yaw, f32 pitch);
        Vec3 get_front();

        void set_frustum(f32 left, f32 right, f32 bottom, f32 top, f32 near,
                         f32 far, bool is_ortho);

        void set_perspective(f32 fovy, f32 aspect, f32 near, f32 far);
        void set_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near,
                       f32 far);
        const Frustum &get_frustum();
        Mat4 look_at();
        Mat4 projection();
        Vec3 to_world_pos(Vec2 pos);

        Camera(Vec3 pos, Vec3 up, Vec3 front);
        Camera();
        ~Camera() = default;
};

}  // namespace Seed

#endif