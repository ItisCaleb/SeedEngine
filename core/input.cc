#include "input.h"
#include "core/engine.h"

namespace Seed {

void Input::reset_input() { key_pressed.clear(); }

bool Input::is_key_pressed(KeyCode code) { return key_pressed.count(code); }

bool Input::is_key_clicked(KeyCode code) {
    return key_pressed.count(code) && !last_key_pressed.count(code);
}

void Input::on_mouse_move(
    std::function<void(f32 last_x, f32 last_y, f32 x, f32 y)> cb) {
    this->drag_func = cb;
}

void Input::mouse_click(MouseEvent e) {}

bool Input::is_mouse_clicked(MouseEvent e) {
    return mouse_pressed.count(e) && !last_mouse_pressed.count(e);
}
bool Input::is_mouse_pressed(MouseEvent e) { return mouse_pressed.count(e); }

bool Input::is_mouse_released(MouseEvent e) {
    return last_mouse_pressed.count(e) && !mouse_pressed.count(e);
}

Vec2i Input::get_mouse_actual_pos() {
    RectF dim =
        System::gEngine->get_window()->get_viewport().get_actual_dimension();
    return Vec2i{(i32)(last_x * dim.w), (i32)(last_y * dim.h)};
}

Input::Input() {}
Input::~Input() {}
}  // namespace Seed
