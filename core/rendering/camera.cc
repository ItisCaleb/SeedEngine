#include "camera.h"
#include "core/math/utils.h"
#include <spdlog/spdlog.h>
#include "core/math/mat3.h"

namespace Seed {
void Camera::set_position(Vec3 pos) {
    if (this->position == pos) {
        return;
    }
    this->position = pos;
    dirty = true;
}
Vec3 Camera::get_position() { return this->position; }
void Camera::set_up(Vec3 up) {
    if (this->up == up) {
        return;
    }
    this->up = up;
    dirty = true;
}
Vec3 Camera::get_up() { return this->up; }
void Camera::set_front(Vec3 front) {
    this->front = front.norm();
    dirty = true;
}
void Camera::set_front(f32 yaw, f32 pitch) {
    Vec3 dir;
    dir.x = sin(radians(yaw)) * cos(radians(pitch));
    dir.y = sin(radians(pitch));
    dir.z = -cos(radians(yaw)) * cos(radians(pitch));
    this->set_front(dir);
}
Vec3 Camera::get_front() { return this->front; }

void Camera::calculate_frustum() {
    Vec3 w = -front;
    /* right */
    Vec3 u = up.cross(w).norm();
    /* vup */
    Vec3 v = w.cross(u).norm();
    if (frustum.is_ortho) {
        frustum_plane.right = {.point = position + u * frustum.right,
                               .normal = -u};
        frustum_plane.left = {.point = position + u * frustum.left,
                              .normal = u};
        frustum_plane.top = {.point = position + v * frustum.top, .normal = -v};
        frustum_plane.bottom = {.point = position + v * frustum.bottom,
                                .normal = v};
        frustum_plane.near = {.point = position + front * frustum.near,
                              .normal = front};
        frustum_plane.far = {.point = position + front * frustum.far,
                             .normal = -front};
    } else {
        frustum_plane.right = {
            .point = position,
            .normal = v.cross(front * frustum.near + u * frustum.right).norm()};
        frustum_plane.left = {
            .point = position,
            .normal =
                (front * frustum.near + u * frustum.left).cross(v).norm()};
        frustum_plane.top = {
            .point = position,
            .normal = (front * frustum.near + v * frustum.top).cross(u).norm()};
        frustum_plane.bottom = {
            .point = position,
            .normal =
                u.cross(front * frustum.near + v * frustum.bottom).norm()};
        frustum_plane.near = {.point = position + front * frustum.near,
                              .normal = front};
        frustum_plane.far = {.point = position + front * frustum.far,
                             .normal = -front};
    }
}

void Camera::set_frustum(f32 left, f32 right, f32 bottom, f32 top, f32 near,
                         f32 far, bool is_ortho) {
    frustum.left = left;
    frustum.right = right;
    frustum.bottom = bottom;
    frustum.top = top;
    frustum.near = near;
    frustum.far = far;
    frustum.is_ortho = is_ortho;
    dirty = true;
}

void Camera::set_perspective(f32 fovy, f32 aspect, f32 near, f32 far) {
    f32 h = near * tan(radians(fovy) / 2);
    f32 w = h * aspect;
    set_frustum(-w, w, -h, h, near, far, false);
}

void Camera::set_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near,
                       f32 far) {
    set_frustum(left, right, bottom, top, near, far, true);
}

Mat4 Camera::look_at() {
    calculate_dirty();
    return this->lookat_mat;
}

void Camera::calculate_lookat() {
    Vec3 w = -front;
    Vec3 u = up.cross(w).norm();
    Vec3 v = w.cross(u).norm();
    this->lookat_mat =
        Mat4::coord_transform_mat(u, v, w) * Mat4::translate_mat(-position);
}
void Camera::calculate_dirty() {
    if (this->dirty) {
        calculate_lookat();
        calculate_frustum();
        this->dirty = false;
    }
}

Mat4 Camera::projection() {
    if (this->frustum.is_ortho) {
        return Mat4::ortho_mat(frustum.right, frustum.left, frustum.top,
                               frustum.bottom, frustum.near, frustum.far);

    } else {
        f32 w = frustum.right - frustum.left;
        f32 rl = frustum.right + frustum.left;
        f32 h = frustum.top - frustum.bottom;
        f32 tb = frustum.top + frustum.bottom;
        f32 d = frustum.far - frustum.near;
        f32 fn = frustum.far + frustum.near;
        return Mat4({Vec4{2 * frustum.near / w, 0, rl / w, 0},
                     Vec4{0, 2 * frustum.near / h, tb / h, 0},
                     Vec4{0, 0, -fn / d, -2 * frustum.far * frustum.near / d},
                     Vec4{0, 0, -1, 0}});
    }
}

const Frustum &Camera::get_frustum() {
    this->calculate_dirty();
    return frustum_plane;
}

Vec3 Camera::to_world_pos(Vec2 pos) { return {}; }

Camera::Camera(Vec3 pos, Vec3 up, Vec3 front)
    : position(pos), up(up), front(front.norm()) {}

Camera::Camera()
    : position(Vec3{0, 0, 0}), up(Vec3{0, 1, 0}), front(Vec3{0, 0, -1}) {}
}  // namespace Seed