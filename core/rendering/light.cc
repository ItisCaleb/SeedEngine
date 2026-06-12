#include "light.h"

namespace Seed {

void DirectionalLight::calculate_csm_lightspace(
    Camera &cam, const std::vector<f32> &resolutions, CSMShadow &csm_data,
    Camera::ShaderCamera *light_space_cam) {
    u32 splits = resolutions.size();
    const Vec3 up = Vec3{0, 1, 0};

    Vec3 cam_front = cam.get_front();
    Vec3 cam_pos = cam.get_position();

    /* localspace w u v*/
    Vec3 w = Vec3{0, 0, 1};
    /* right */
    Vec3 u = up.cross(w).norm();
    /* vup */
    Vec3 v = w.cross(u).norm();
    f32 lambda = shadow_lamdba;
    f32 n0 = cam.frustum.near;
    f32 f0 = 1000;
    f32 near = n0;

    /* light lookat matrix */
    Vec3 light_w = -dir.norm();
    Vec3 light_front = -light_w;
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
    for (u32 i = 1; i <= splits; i++) {
        /* we calculate frusta sphere at local space */
        f32 far = lambda * n0 * powf((f0 / n0), i / (f32)splits) +
                  (1 - lambda) * (n0 + (i / (f32)splits) * (f0 - n0));
        f32 n_r = near / n0 * cam.frustum.right;
        f32 n_t = near / n0 * cam.frustum.top;
        f32 f_r = far / n0 * cam.frustum.right;
        f32 f_t = far / n0 * cam.frustum.top;
        Vec3 n_right = u * n_r;
        Vec3 n_top = v * n_t;
        Vec3 f_right = u * f_r;
        Vec3 f_top = v * f_t;

        Vec3 nv = (n_right + n_top) * 2;
        Vec3 fv = (f_right + f_top) * 2;

        /* Just use Pythagorean theorem to calculate center and radius */
        /* |a|^2 + x^2 = r^2 = |b|^2 + (len - x)^2 */
        /* a = 2 * vector to near top right, b = vector to far top right */
        f32 len = far - near;
        f32 a2 = nv.lensq();
        f32 b2 = fv.lensq();
        f32 x = len / 2 + (b2 - a2) / (8 * len);
        f32 radius = sqrtf(a2 / 4 + x * x);
        f32 AABB_size = radius * 2;
        f32 unit = AABB_size / resolutions[i - 1];
        Mat4 light_projection = Mat4::ortho_mat_zero(radius, -radius, radius,
                                                     -radius, -radius, radius);

        Vec3 center = cam_pos + cam_front * (near + x);
        /* transform center to light space and quantize */
        Vec4 center_ls = light_lookat * Vec4{center.x, center.y, center.z, 1.0};
        center_ls.x = floorf(center_ls.x / unit) * unit;
        center_ls.y = floorf(center_ls.y / unit) * unit;

        /* transform back */
        Vec4 center_ws = light_lookat.transpose() * center_ls;
        center.x = center_ws.x;
        center.y = center_ws.y;
        center.z = center_ws.z;

        Mat4 light_view = light_lookat * Mat4::translate_mat(-center);
        light_space_cam[i - 1].projection = light_projection;
        light_space_cam[i - 1].view = light_view;
        csm_data.fars[i - 1] = far;
        csm_data.units[i - 1] = unit;
        /* we muliply by 100 to prevent error culled */
        frustum_cache[i - 1] = {
            .left = {.point = center - light_u * radius, .normal = light_u},
            .right = {.point = center + light_u * radius, .normal = -light_u},
            .top = {.point = center + light_v * radius, .normal = -light_v},
            .bottom = {.point = center - light_v * radius, .normal = light_v},
            .near = {.point = center - light_front * radius * 100,
                     .normal = light_front},
            .far = {.point = center + light_front * radius,
                    .normal = -light_front},
        };

        /* next split */
        near = far;
    }
}

void PointLight::calculate_lightspace(Camera::ShaderCamera *light_space_cam) {
    if (dirty) {
        Camera cam;
        cam.set_position(this->pos);
        cam.set_up(Vec3{0, 1, 0});
        cam.set_perspective(90, 1, 1.0f, 25.0f);
        light_space_proj = cam.projection_zero();
        /* z -z x -x y -y*/
        cam.set_front(Vec3{0, 0, 1});
        light_space_lookat[0] = cam.look_at();
        frustum_cache[0] = cam.get_frustum();
        cam.set_front(Vec3{0, 0, -1});
        light_space_lookat[1] = cam.look_at();
        frustum_cache[1] = cam.get_frustum();
        cam.set_front(Vec3{1, 0, 0});
        light_space_lookat[2] = cam.look_at();
        frustum_cache[2] = cam.get_frustum();

        cam.set_front(Vec3{-1, 0, 0});
        light_space_lookat[3] = cam.look_at();
        frustum_cache[3] = cam.get_frustum();

        /* set up to horizontal */
        cam.set_up(Vec3{1, 0, 0});
        cam.set_front(Vec3{0, 1, 0});
        light_space_lookat[4] = cam.look_at();
        frustum_cache[4] = cam.get_frustum();

        cam.set_front(Vec3{0, -1, 0});
        light_space_lookat[5] = cam.look_at();
        frustum_cache[5] = cam.get_frustum();

        dirty = false;
    }
    for (u32 i = 0; i < 6; i++) {
        light_space_cam[i].projection = light_space_proj;
        light_space_cam[i].view = light_space_lookat[i];
    }
}
};  // namespace Seed