#include "camera_entity.h"

#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>

#include "core/engine.h"
#include "core/input.h"
#include "core/ref.h"
#include "core/world/entity.h"
#include "editor/editor.h"

namespace Seed {

namespace {

constexpr f32 kMouseSensitivity = 0.12f;
constexpr f32 kMinimumSpeed = 25.0f;
constexpr f32 kMaximumSpeed = 20000.0f;
constexpr f32 kSpeedStep = 1.2f;
constexpr f32 kBoostMultiplier = 4.0f;
constexpr f32 kPanScale = 0.002f;
constexpr f32 kMinimumOrbitDistance = 1.0f;
constexpr f32 kMaximumOrbitDistance = 100000.0f;
constexpr f32 kFocusDistance = 100.0f;

GLFWwindow *get_editor_window() {
    SeedEngine *engine = SeedEngine::get_instance();
    if (engine == nullptr || engine->get_window() == nullptr) return nullptr;
    return engine->get_window()->get_window<GLFWwindow>();
}

bool is_key_pressed(GLFWwindow *window, i32 key) {
    return glfwGetKey(window, key) == GLFW_PRESS;
}

}  // namespace

void EditorCameraBehaviour::start() {
    this->cam = &SeedEngine::get_instance()->get_world()->get_camera();
    this->cam->set_position(Vec3{0, 750, 1000});
    this->cam->set_front(0, -50);
    this->cam->set_perspective(45, 1.33, 0.1, 10000.0);
    yaw = 0;
    pitch = -50;
    speed = 1000;
    orbit_center = cam->get_position() + cam->get_front() * orbit_distance;
}

bool EditorCameraBehaviour::begin_navigation(NavigationMode mode) {
    if (gEditor == nullptr || !gEditor->world_editor.is_viewport_hovered()) {
        return false;
    }

    GLFWwindow *window = get_editor_window();
    if (window == nullptr) return false;

    glfwGetCursorPos(window, &last_mouse_x, &last_mouse_y);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    navigation_mode = mode;
    return true;
}

void EditorCameraBehaviour::end_navigation() {
    GLFWwindow *window = get_editor_window();
    if (window != nullptr) {
        if (glfwRawMouseMotionSupported()) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    navigation_mode = NavigationMode::None;
}

void EditorCameraBehaviour::rotate(f32 delta_x, f32 delta_y) {
    yaw += delta_x * kMouseSensitivity;
    pitch = std::clamp(pitch - delta_y * kMouseSensitivity, -89.0f, 89.0f);
    cam->set_front(yaw, pitch);
}

void EditorCameraBehaviour::focus(Vec3 target) {
    orbit_center = target;
    orbit_distance = kFocusDistance;
    cam->set_position(orbit_center - cam->get_front() * orbit_distance);
}

void EditorCameraBehaviour::update(f32 dt) {
    Input *input = Input::get_instance();
    GLFWwindow *window = get_editor_window();
    if (cam == nullptr || input == nullptr || window == nullptr ||
        gEditor == nullptr) {
        return;
    }

    const bool alt_pressed =
        is_key_pressed(window, GLFW_KEY_LEFT_ALT) ||
        is_key_pressed(window, GLFW_KEY_RIGHT_ALT);
    if (navigation_mode == NavigationMode::None &&
        gEditor->world_editor.is_viewport_hovered()) {
        if (alt_pressed && input->is_mouse_clicked(MouseEvent::LEFT)) {
            begin_navigation(NavigationMode::Orbit);
        } else if (input->is_mouse_clicked(MouseEvent::MIDDLE)) {
            begin_navigation(NavigationMode::Pan);
        } else if (input->is_mouse_clicked(MouseEvent::RIGHT)) {
            begin_navigation(NavigationMode::Fly);
        }
    }

    bool navigation_held = true;
    switch (navigation_mode) {
        case NavigationMode::Fly:
            navigation_held = input->is_mouse_pressed(MouseEvent::RIGHT);
            break;
        case NavigationMode::Pan:
            navigation_held = input->is_mouse_pressed(MouseEvent::MIDDLE);
            break;
        case NavigationMode::Orbit:
            navigation_held = alt_pressed &&
                              input->is_mouse_pressed(MouseEvent::LEFT);
            break;
        case NavigationMode::None:
            break;
    }
    if (navigation_mode != NavigationMode::None &&
        (!navigation_held ||
         !glfwGetWindowAttrib(window, GLFW_FOCUSED))) {
        end_navigation();
    }

    if (input->is_key_clicked(KeyCode::F) &&
        gEditor->world_editor.is_viewport_hovered()) {
        Vec3 target;
        if (gEditor->world_editor.get_camera_focus(target)) focus(target);
    }

    const f32 scroll = gEditor->world_editor.consume_viewport_scroll();
    if (scroll != 0.0f) {
        if (navigation_mode == NavigationMode::Fly) {
            speed *= std::pow(kSpeedStep, -scroll);
            speed = std::clamp(speed, kMinimumSpeed, kMaximumSpeed);
        } else {
            orbit_distance = std::clamp(
                orbit_distance * std::pow(kSpeedStep, scroll),
                kMinimumOrbitDistance, kMaximumOrbitDistance);
            cam->set_position(orbit_center -
                              cam->get_front() * orbit_distance);
        }
    }

    if (navigation_mode == NavigationMode::None) return;

    f64 mouse_x = 0.0;
    f64 mouse_y = 0.0;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    const f32 delta_x = (f32)(mouse_x - last_mouse_x);
    const f32 delta_y = (f32)(mouse_y - last_mouse_y);
    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    Vec3 pos = cam->get_position();
    Vec3 front = cam->get_front().norm();
    Vec3 right = front.cross(cam->get_up()).norm();
    Vec3 up = cam->get_up().norm();

    if (navigation_mode == NavigationMode::Orbit) {
        rotate(delta_x, delta_y);
        cam->set_position(orbit_center -
                          cam->get_front() * orbit_distance);
        return;
    }

    if (navigation_mode == NavigationMode::Pan) {
        const f32 scale = std::max(orbit_distance * kPanScale, 0.05f);
        const Vec3 screen_up = right.cross(front).norm();
        const Vec3 movement =
            right * (-delta_x * scale) + screen_up * (delta_y * scale);
        cam->set_position(pos + movement);
        orbit_center += movement;
        return;
    }

    rotate(delta_x, delta_y);
    front = cam->get_front().norm();
    right = front.cross(cam->get_up()).norm();
    const bool boost = is_key_pressed(window, GLFW_KEY_LEFT_SHIFT) ||
                       is_key_pressed(window, GLFW_KEY_RIGHT_SHIFT);
    const f32 step = speed * (boost ? kBoostMultiplier : 1.0f) * dt;

    if (input->is_key_pressed(KeyCode::W)) pos += front * step;
    if (input->is_key_pressed(KeyCode::S)) pos -= front * step;
    if (input->is_key_pressed(KeyCode::D)) pos += right * step;
    if (input->is_key_pressed(KeyCode::A)) pos -= right * step;
    if (input->is_key_pressed(KeyCode::E)) pos += up * step;
    if (input->is_key_pressed(KeyCode::Q)) pos -= up * step;

    cam->set_position(pos);
    orbit_center = pos + cam->get_front() * orbit_distance;
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
