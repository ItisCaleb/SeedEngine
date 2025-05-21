#include "input.h"
#include <spdlog/spdlog.h>

namespace Seed {
Input *Input::get_instance() { return instance; }

void Input::reset_input() { key_pressed.clear(); }

bool Input::is_key_pressed(KeyCode code) { return key_pressed.count(code); }

void Input::on_mouse_move(
    std::function<void(f32 last_x, f32 last_y, f32 x, f32 y)> cb) {
    this->drag_func = cb;
}

void mouse_click(MouseEvent e) {}

bool Input::is_mouse_clicked(MouseEvent e) { return mouse_pressed.count(e); }

Input::Input() { instance = this; }
Input::~Input() { instance = nullptr; }
}  // namespace Seed
