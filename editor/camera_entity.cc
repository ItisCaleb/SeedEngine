#include "camera_entity.h"
#include "core/input.h"
#include "core/engine.h"
#include <imgui.h>
#include "core/ref.h"
#include "core/world/entity.h"

namespace Seed {

void EditorCameraBehaviour::start() {
    this->cam = &SeedEngine::get_instance()->get_world()->get_camera();
    this->cam->set_position(Vec3{0, 750, 1000});
    this->cam->set_front(0, -50);
    this->cam->set_perspective(45, 1.33, 0.1, 10000.0);
    yaw = 0;
    pitch = -50;
    speed = 1000;
}
void EditorCameraBehaviour::update(f32 dt) {
    Input *input = Input::get_instance();
    if (!input->is_mouse_pressed(MouseEvent::RIGHT)) return;

    ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
    f32 sensitivity = 0.1f;
    yaw += mouse_delta.x * sensitivity;
    pitch -= mouse_delta.y * sensitivity;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    cam->set_front(yaw, pitch);

    if (input->is_key_pressed(KeyCode::MINUS)) speed -= 500 * dt;
    if (input->is_key_pressed(KeyCode::EQUAL)) speed += 500 * dt;
    if (speed < 50) speed = 50;

    Vec3 pos = cam->get_position();
    Vec3 front = cam->get_front().norm();
    Vec3 right = front.cross(cam->get_up()).norm();
    Vec3 up = cam->get_up().norm();
    f32 step = speed * dt;

    if (input->is_key_pressed(KeyCode::W)) pos += front * step;
    if (input->is_key_pressed(KeyCode::S)) pos -= front * step;
    if (input->is_key_pressed(KeyCode::D)) pos += right * step;
    if (input->is_key_pressed(KeyCode::A)) pos -= right * step;
    if (input->is_key_pressed(KeyCode::E)) pos += up * step;
    if (input->is_key_pressed(KeyCode::Q)) pos -= up * step;

    cam->set_position(pos);
}

Entity EditorCameraEntity::create_entity(EntityManager &m) {
    Entity e = m.create_entity();
    Ref<EditorCameraBehaviour> b;
    b.create();
    m.add_component<BehaviourComponent>(
        e, BehaviourComponent{.behaviour = ref_cast<Behaviour>(b)});
    return e;
}
void EditorCameraEntity::destroy_entity(EntityManager &m, Entity e) {
    m.remove_component<BehaviourComponent>(e);
    m.destroy_entity(e);
}
}  // namespace Seed
