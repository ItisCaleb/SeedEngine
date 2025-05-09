#include "camera.h"
#include "core/math/utils.h"

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
Vec3 Camera::get_front() { return this->front; }

void Camera::calculate_frustum() {
    Vec3 w = -front;
    /* right */
    Vec3 u = up.cross(w).norm();
    /* vup */
    Vec3 v = w.cross(u).norm();
    if (!frustum.is_ortho) {
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
bool Camera::test_aabb_plane(AABB &aabb, Plane &plane) {
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    float r = aabb.ext.x * std::abs(plane.normal.x) +
              aabb.ext.y * std::abs(plane.normal.y) +
              aabb.ext.z * std::abs(plane.normal.z);

    // Compute distance of box center from plane
    // (n . C) - d = (n . C) - (n . P) = n . (C - P)
    float s = plane.normal.dot(aabb.center - plane.point);

    // Intersection occurs when distance s falls within [-r,+r] interval
    return -r <= s;
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

bool Camera::within_frustum(AABB &bounding_box) {
    this->calculate_dirty();
    return test_aabb_plane(bounding_box, frustum_plane.right) &&
           test_aabb_plane(bounding_box, frustum_plane.left) &&
           test_aabb_plane(bounding_box, frustum_plane.top) &&
           test_aabb_plane(bounding_box, frustum_plane.bottom) &&
           test_aabb_plane(bounding_box, frustum_plane.near) &&
           test_aabb_plane(bounding_box, frustum_plane.far);
}

Camera::Camera(Vec3 pos, Vec3 up, Vec3 front)
    : position(pos), up(up), front(front.norm()) {}

Camera::Camera()
    : position(Vec3{0, 0, 0}), up(Vec3{0, 1, 0}), front(Vec3{0, 0, -1}) {}
}  // namespace Seed