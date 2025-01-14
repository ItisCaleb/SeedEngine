#include <seed/rendering/camera.h>
#include <seed/math/utils.h>

namespace Seed {
void Camera::set_position(Vec3 pos) { this->position = pos; }
Vec3 Camera::get_position() { return this->position; }
void Camera::set_up(Vec3 up) { this->up = up; }
Vec3 Camera::get_up() { return this->up; }
void Camera::set_front(Vec3 front) { this->front = front; }
Vec3 Camera::get_front() { return this->front; }

void Camera::set_frustum(f32 left, f32 right, f32 bottom, f32 top, f32 near,
                         f32 far, bool is_ortho) {
    frustum.left = left;
    frustum.right = right;
    frustum.bottom = bottom;
    frustum.top = top;
    frustum.near = near;
    frustum.far = far;
    frustum.is_ortho = is_ortho;
}
void Camera::set_perspective(f64 fovy, f64 aspect, f64 near, f64 far) {
    f64 h = near * tan(to_radians(fovy) / 2);
    f64 w = h * aspect;
    set_frustum(-w, w, -h, h, near, far, false);
}

Mat4 Camera::look_at() {
    Vec3 w = -front;
    Vec3 u = front.cross(up);
    Vec3 v = w.cross(u);
    return Mat4::coord_transform_mat(u, v, w) * Mat4::translate_mat(-position);
}

Mat4 Camera::perspective() {
    f32 w = frustum.right - frustum.left;
    f32 h = frustum.top - frustum.bottom;
    f32 d = frustum.far - frustum.near;
    return Mat4(
        {Vec4{2 * frustum.near / w, 0, (frustum.right + frustum.left) / w, 0},
         Vec4{0, 2 * frustum.near / h, (frustum.top + frustum.bottom) / h, 0},
         Vec4{0, 0, -(frustum.far + frustum.near) / d,
              -2 * frustum.far * frustum.near / d},
         Vec4{0, 0, -1, 0}});
}

Camera::Camera(Vec3 pos, Vec3 up, Vec3 front)
    : position(pos), up(up.norm()), front(front.norm()) {}
}  // namespace Seed