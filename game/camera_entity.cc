#include "camera_entity.h"
#include "core/input.h"
#include "core/rendering/api/render_engine.h"
#include <fmt/core.h>
#include "core/math/utils.h"

namespace Seed {
void CameraEntity::update(f32 dt) {
    Input *input = Input::get_instance();
    Vec3 pos = cam->get_position();
    f32 speed = 50 * dt;
    Vec3 front = cam->get_front();
    front.y = 0;
    front = front.norm();
    if (input->is_key_pressed(KeyCode::W)) {
        pos += front * speed;
    }

    if (input->is_key_pressed(KeyCode::S)) {
        pos -= front * speed;
    }

    if (input->is_key_pressed(KeyCode::D)) {
        pos += front.cross(cam->get_up()) * speed;
    }

    if (input->is_key_pressed(KeyCode::A)) {
        pos -= front.cross(cam->get_up()) * speed;
    }
    if (input->is_key_pressed(KeyCode::SPACE)){
        pos += cam->get_up() * speed;
    }
        if (input->is_key_pressed(KeyCode::X)){
        pos -= cam->get_up() * speed;
    }

    cam->set_position(pos);
}

void CameraEntity::render() {

}

CameraEntity::CameraEntity() {
    
    this->cam = RenderEngine::get_instance()->get_cam();
    this->cam->set_position(Vec3{0, 20 ,0});
    this->cam->set_perspective(45, 1.33, 0.1, 10000.0);
    Input::get_instance()->on_mouse_move([=](i32 last_x, i32 last_y, i32 x, i32 y){
        if(!Input::get_instance()->is_mouse_clicked(MouseEvent::LEFT)){
            return;
        }
        f32 x_off = x - last_x;
        f32 y_off = last_y - y;
        f32 sensitivity = 0.15f;
        x_off *= sensitivity;
        y_off *= sensitivity;
        yaw += x_off;
        pitch += y_off;

        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;
        Vec3 dir;
        dir.x = cos(radians(yaw)) * cos(radians(pitch));
        dir.y = sin(radians(pitch));
        dir.z = sin(radians(yaw)) * cos(radians(pitch));
        this->cam->set_front(dir.norm());
    });
}
}  // namespace Seed
