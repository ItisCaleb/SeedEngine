#include "camera_entity.h"
#include "core/input.h"
#include "core/rendering/api/render_engine.h"
#include <fmt/core.h>

namespace Seed {
void CameraEntity::update(f32 dt) {
    Input *input = Input::get_instance();
    Vec3 pos = cam.get_position();
    f32 speed = 5 * dt;
    if (input->is_key_pressed(KeyCode::W)) {
        pos += cam.get_front() * speed;
    }

    if (input->is_key_pressed(KeyCode::S)) {
        pos -= cam.get_front() * speed;
    }

    if (input->is_key_pressed(KeyCode::D)) {
        pos += cam.get_front().cross(cam.get_up()) * speed;
    }

    if (input->is_key_pressed(KeyCode::A)) {
        pos -= cam.get_front().cross(cam.get_up()) * speed;
    }
    cam.set_position(pos);
}

void CameraEntity::render(RenderCommandDispatcher &dp){
    dp.begin();
    Mat4* matrices = (Mat4*)dp.update(matrices_rc, 0, sizeof(Mat4) * 2);
    matrices[0] = cam.perspective().transpose();
    matrices[1] = cam.look_at().transpose();
    Vec3* cam_pos = (Vec3*)dp.update(cam_rc, 0, sizeof(Vec3));
    *cam_pos = this->get_position();
    dp.end();
}

CameraEntity::CameraEntity() {
    this->cam.set_perspective(45, 1.33, 0.1, 1000.0);
    this->matrices_rc = &RenderResource::global_resource["Matrices"];
    this->cam_rc = &RenderResource::global_resource["Camera"];

}
}  // namespace Seed
