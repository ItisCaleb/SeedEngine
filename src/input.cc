#include <seed/input.h>
#include <spdlog/spdlog.h>

namespace Seed
{
    Input* Input::get_instance(){
        return instance;
    }

    void Input::reset_input(){
        key_pressed.clear();
    }

    void Input::press_key(KeyCode code){
        key_pressed.insert(code);
    }

    void Input::release_key(KeyCode code){
        key_pressed.erase(code);
    }
    
    bool Input::is_key_pressed(KeyCode code){
        return key_pressed.count(code);
    }


    Input::Input(){
        instance = this;
        spdlog::info("Initializing Input");
    }
    Input::~Input(){
        instance = nullptr;
    }
} // namespace Seed
