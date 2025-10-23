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
bool Camera::test_aabb_plane(const AABB &aabb, const Plane &plane) {
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

float Camera::calculate_depth(const Vec3 &pos) {
    float dist = (pos - this->position).length();
    return dist / frustum.far;
}

Mat4 Camera::projection() {
    f32 w = frustum.right - frustum.left;
    f32 rl = frustum.right + frustum.left;
    f32 h = frustum.top - frustum.bottom;
    f32 tb = frustum.top + frustum.bottom;
    f32 d = frustum.far - frustum.near;
    f32 fn = frustum.far + frustum.near;
    if (this->frustum.is_ortho) {
        return Mat4({Vec4{2 / w, 0, 0, -rl / w}, Vec4{0, 2 / h, 0, -tb / h},
                     Vec4{0, 0, -2 / d, -fn / d}, Vec4{0, 0, 0, 1}});
    } else {
        return Mat4({Vec4{2 * frustum.near / w, 0, rl / w, 0},
                     Vec4{0, 2 * frustum.near / h, tb / h, 0},
                     Vec4{0, 0, -fn / d, -2 * frustum.far * frustum.near / d},
                     Vec4{0, 0, -1, 0}});
    }
}

bool Camera::within_frustum(const AABB &bounding_box) {
    this->calculate_dirty();
    return test_aabb_plane(bounding_box, frustum_plane.right) &&
           test_aabb_plane(bounding_box, frustum_plane.left) &&
           test_aabb_plane(bounding_box, frustum_plane.top) &&
           test_aabb_plane(bounding_box, frustum_plane.bottom) &&
           test_aabb_plane(bounding_box, frustum_plane.near) &&
           test_aabb_plane(bounding_box, frustum_plane.far);
}

void Camera::calculate_csm_lightspace(const Vec3 &dir, u8 splits,
                                      std::vector<Mat4> &lightspaces, std::vector<f32> &fars) {
    if (this->frustum.is_ortho) {
        SPDLOG_WARN("Camera must be perspective to calculate CSM.");
        return;
    }
    if (splits > 5) {
        SPDLOG_WARN("Too many CSM splits.");
        return;
    }
    Vec3 w = -front;
    /* right */
    Vec3 u = up.cross(w).norm();
    /* vup */
    Vec3 v = w.cross(u).norm();
    f32 lambda = shadow_lamdba;
    f32 n0 = this->frustum.near;
    f32 f0 = this->frustum.far;
    f32 near = n0;

    /* light lookat matrix */
    Vec3 light_w = -dir.norm();
    Vec3 light_u = up.cross(light_w).norm();
    Vec3 light_v = light_w.cross(light_u).norm();
    Mat4 light_lookat = Mat4::coord_transform_mat(light_u, light_v, light_w);
    Mat4 light_view = light_lookat * Mat4::translate_mat(-position);
    /*
        https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf
        zᵢ = λn(f/n)*(i/N) + (1−λ)(n+(i/N)(f−n))
        zᵢ: current split far
        f: original far
        n: original near
        N: target splits
        λ: correction factor
    */
    std::vector<Vec3> corners;
    for (u32 i = 1; i <= splits; i++) {
        f32 far = lambda * n0 * powf((f0 / n0), i / (f32)splits) +
                  (1 - lambda) * (n0 + (i / (f32)splits) * (f0 - n0));
        Vec3 n_front = front * near;
        Vec3 n_right = u * (near / n0 * this->frustum.right);
        Vec3 n_top = v * (near / n0 * this->frustum.top);
        Vec3 f_front = front * far;
        Vec3 f_right = u * (far / n0 * this->frustum.right);
        Vec3 f_top = v * (far / n0 * this->frustum.top);
        /* near top right */
        corners.push_back(n_front + n_right + n_top);
        /* near bottom right */
        corners.push_back(n_front + n_right - n_top);
        /* near top left */
        corners.push_back(n_front - n_right + n_top);
        /* near bottom left */
        corners.push_back(n_front - n_right - n_top);

        /* far top right */
        corners.push_back(f_front + f_right + f_top);
        /* far bottom right */
        corners.push_back(f_front + f_right - f_top);
        /* far top left */
        corners.push_back(f_front - f_right + f_top);
        /* far bottom left */
        corners.push_back(f_front - f_right - f_top);
        Vec3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
        Vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vec3 center = {0, 0, 0};
        for (Vec3 &corner : corners) {
            center += corner;
        }
        center /= corners.size();
        for (Vec3 &corner : corners) {
            Vec4 _corner = light_lookat * Mat4::translate_mat(-center) *
                           Vec4{corner.x, corner.y, corner.z, 1};
            max.x = std::max(max.x, _corner.x);
            max.y = std::max(max.y, _corner.y);
            max.z = std::max(max.z, _corner.z);
            min.x = std::min(min.x, _corner.x);
            min.y = std::min(min.y, _corner.y);
            min.z = std::min(min.z, _corner.z);
        }
        f32 w = max.x - min.x;
        f32 h = max.y - min.y;
        f32 d = max.z - min.z;
        f32 rl = max.x + min.x;
        f32 tb = max.y + min.y;
        f32 fn = max.z + min.z;
        Mat4 light_projection =
            Mat4({Vec4{2 / w, 0, 0, -rl / w}, Vec4{0, 2 / h, 0, -tb / h},
                  Vec4{0, 0, -2 / d, -fn / d}, Vec4{0, 0, 0, 1}});
        lightspaces.push_back(light_projection * light_view);
        fars.push_back(far);

        /* next split */
        near = far;
        corners.clear();
    }
}

Vec3 Camera::to_world_pos(Vec2 pos) { return {}; }

Camera::Camera(Vec3 pos, Vec3 up, Vec3 front)
    : position(pos), up(up), front(front.norm()) {}

Camera::Camera()
    : position(Vec3{0, 0, 0}), up(Vec3{0, 1, 0}), front(Vec3{0, 0, -1}) {}
}  // namespace Seed