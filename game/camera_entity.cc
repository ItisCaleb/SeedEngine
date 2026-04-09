#include "camera_entity.h"
#include "core/input.h"
#include "core/engine.h"
#include <fmt/core.h>
#include "core/math/utils.h"
#include "core/ref.h"
#include "core/world/entity.h"

namespace Seed {

void CameraBehaviour::start() {
    this->cam = &SeedEngine::get_instance()->get_world()->get_camera();
    this->cam->set_position(Vec3{0, 20, 0});
    this->cam->set_perspective(45, 1.33, 0.1, 2000.0);
    yaw = 0;
    pitch = 0;
    Input::get_instance()->on_mouse_move(
        [=](f32 last_x, f32 last_y, f32 x, f32 y) {
            if (!Input::get_instance()->is_mouse_pressed(MouseEvent::LEFT)) {
                return;
            }
            f32 x_off = x - last_x;
            f32 y_off = last_y - y;
            f32 sensitivity = 100;
            x_off *= sensitivity;
            y_off *= sensitivity;
            yaw += x_off;
            pitch += y_off;

            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
            this->cam->set_front(yaw, pitch);
        });
}

void CameraBehaviour::update(f32 dt) {
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
    if (input->is_key_pressed(KeyCode::SPACE)) {
        pos += cam->get_up() * speed;
    }
    if (input->is_key_pressed(KeyCode::X)) {
        pos -= cam->get_up() * speed;
    }

    cam->set_position(pos);
}
Entity CameraEntity::create_entity(EntityManager &m) {
    Entity e = m.create_entity();
    Ref<CameraBehaviour> b;
    b.create();
    m.add_component<BehaviourComponent>(
        e, BehaviourComponent{.behaviour = ref_cast<Behaviour>(b)});
    return e;
}
void CameraEntity::destroy_entity(EntityManager &m, Entity e) {
    m.remove_component<BehaviourComponent>(e);
    m.destroy_entity(e);
}
}  // namespace Seed
