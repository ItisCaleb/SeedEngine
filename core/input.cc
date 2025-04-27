#include "input.h"
#include <spdlog/spdlog.h>

namespace Seed {
Input *Input::get_instance() { return instance; }

void Input::reset_input() { key_pressed.clear(); }

void Input::press_key(KeyCode code) { key_pressed.insert(code); }

void Input::release_key(KeyCode code) { key_pressed.erase(code); }

bool Input::is_key_pressed(KeyCode code) { return key_pressed.count(code); }

void Input::on_mouse_move(std::function<void(i32 last_x, i32 last_y, i32 x, i32 y)> cb){
    this->drag_func = cb;
}

void Input::mouse_move(i32 x, i32 y){
    if(drag_func){
        drag_func(last_x, last_y, x, y);
    }
    last_x = x;
    last_y = y;
}

Input::Input() {
    instance = this;
    spdlog::info("Initializing Input");
}
Input::~Input() { instance = nullptr; }
}  // namespace Seed
