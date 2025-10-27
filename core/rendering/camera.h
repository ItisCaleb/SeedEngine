#ifndef _SEED_CAMERA_H_
#define _SEED_CAMERA_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/math/mat4.h"
#include "core/collision/shape.h"
#include "core/collision/aabb.h"
#include "core/rendering/light.h"
#include <vector>

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
        bool test_aabb_plane(const AABB &aabb, const Plane &plane);
        void calculate_frustum();
        void calculate_lookat();
        void calculate_dirty();

    public:
        static f32 shadow_lamdba;
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
        bool within_frustum(const AABB &bounding_box);
        Mat4 look_at();
        Mat4 projection();
        Vec3 to_world_pos(Vec2 pos);
        float calculate_depth(const Vec3 &pos);
        void calculate_csm_lightspace(const Vec3 &dir,
                                      const std::vector<f32> &resolutions,
                                      CSMShadow &csm_data);

        Camera(Vec3 pos, Vec3 up, Vec3 front);
        Camera();
        ~Camera() = default;
};

}  // namespace Seed

#endif