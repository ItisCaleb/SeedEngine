#ifndef _SEED_CAMERA_H_
#define _SEED_CAMERA_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/math/mat4.h"
#include "core/collision/plane.h"
#include "core/collision/aabb.h"

namespace Seed {
class Camera {
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
        struct {
                Plane left, right;
                Plane top, bottom;
                Plane near, far;
        } frustum_plane;
        Mat4 lookat_mat;
        bool dirty = true;
        bool test_aabb_plane(AABB &aabb, Plane &plane);
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
        bool within_frustum(AABB &bounding_box);
        Mat4 look_at();
        Mat4 perspective();
        Vec3 to_world_pos(Vec2 pos);

        Camera(Vec3 pos, Vec3 up, Vec3 front);
        Camera();
        ~Camera() = default;
};

}  // namespace Seed

#endif