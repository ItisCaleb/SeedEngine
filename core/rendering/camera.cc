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

bool Camera::within_frustum(const AABB &bounding_box) {
    this->calculate_dirty();
    return test_aabb_plane(bounding_box, frustum_plane.right) &&
           test_aabb_plane(bounding_box, frustum_plane.left) &&
           test_aabb_plane(bounding_box, frustum_plane.top) &&
           test_aabb_plane(bounding_box, frustum_plane.bottom) &&
           test_aabb_plane(bounding_box, frustum_plane.near) &&
           test_aabb_plane(bounding_box, frustum_plane.far);
}

f32 Camera::shadow_lamdba = 0.5;

void Camera::calculate_csm_lightspace(const Vec3 &dir, u8 splits,
                                      std::vector<Mat4> &lightspaces,
                                      std::vector<f32> &fars) {
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
        f32 n_r = near / n0 * this->frustum.right;
        f32 n_t = near / n0 * this->frustum.top;
        f32 f_r = far / n0 * this->frustum.right;
        f32 f_t = far / n0 * this->frustum.top;
        Vec3 n_front = Vec3{near, 0, 0};
        Vec3 n_right = u * n_r;
        Vec3 n_top = v * n_t;
        Vec3 f_front = Vec3{far, 0, 0};
        Vec3 f_right = u * f_r;
        Vec3 f_top = v * f_t;
        /* We add camera position to corners to quantitize after */
        /* near top right */
        corners.push_back(position + n_front + n_right + n_top);
        /* near bottom right */
        corners.push_back(position + n_front + n_right - n_top);
        /* near top left */
        corners.push_back(position + n_front - n_right + n_top);
        /* near bottom left */
        corners.push_back(position + n_front - n_right - n_top);

        /* far top right */
        corners.push_back(position + f_front + f_right + f_top);
        /* far bottom right */
        corners.push_back(position + f_front + f_right - f_top);
        /* far top left */
        corners.push_back(position + f_front - f_right + f_top);
        /* far bottom left */
        corners.push_back(position + f_front - f_right - f_top);
        Vec3 center = {0, 0, 0};
        for (Vec3 &corner : corners) {
            center += corner;
        }
        center /= corners.size();

        /* TODO: Calculate frusta bound sphere */
        f32 radius = (far - near / 2);
        f32 AABB_size = radius * 2;
        f32 unit = AABB_size / 2048.0f;
        Mat4 light_projection =
            Mat4::ortho_mat(radius, -radius, radius, -radius, -radius, radius);

        /* transform center to light space */
        Vec4 center_ls = light_lookat * Vec4{center.x, center.y, center.z, 1.0};
        center_ls.x -= fmodf(center_ls.x, unit);
        center_ls.y -= fmodf(center_ls.y, unit);

        /* transform back */
        Vec4 center_ws = light_lookat.transpose() * center_ls;
        center.x = center_ws.x;
        center.y = center_ws.y;

        Mat4 light_view = light_lookat * Mat4::translate_mat(-center);

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