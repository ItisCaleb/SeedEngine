#include "input_handler.h"
#include "core/input.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace Seed {

void InputHandler::init(Window *window) {
    spdlog::info("Initializing Input handler");
    if (!window) {
        SPDLOG_ERROR(
            "Can't initialize Input handler, window is null, exiting.");
        exit(1);
    }
    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();
    glfwSetKeyCallback(glfw_window, [](GLFWwindow *window, int key, int scancode,
                                  int action, int mods) {
        KeyCode k = static_cast<KeyCode>(key);
        Input *input = Input::get_instance();
        if (action == GLFW_PRESS) {
            input->key_pressed.insert(k);
        } else if (action == GLFW_RELEASE) {
            input->key_pressed.erase(k);
        }
    });
    glfwSetCursorPosCallback(
        glfw_window, [](GLFWwindow *window, double x, double y) {
            Input *input = Input::get_instance();
            if(!input->should_capture_mouse){
                return;
            }
            if (input->drag_func) {
                input->drag_func(input->last_x, input->last_y, x, y);
            }
            input->last_x = x;
            input->last_y = y;
        });
    glfwSetMouseButtonCallback(
        glfw_window, [](GLFWwindow *window, int button, int action, int mods) {
            Input *input = Input::get_instance();
            MouseEvent me = MouseEvent::LEFT;
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                me = MouseEvent::LEFT;
            } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                me = MouseEvent::MIDDLE;
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                me = MouseEvent::RIGHT;
            }
            if (action == GLFW_PRESS) {
                input->mouse_pressed.insert(me);
            } else {
                input->mouse_pressed.erase(me);
            }
        });
}
}  // namespace Seed
