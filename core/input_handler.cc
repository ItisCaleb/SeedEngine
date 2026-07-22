#include "input_handler.h"
#include "core/input.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "core/gui/gui_engine.h"

namespace Seed {
static Window *input_window;

static int modifier_state(GLFWwindow *window, int mods = 0) {
    int state = 0;
    if ((mods & GLFW_MOD_CONTROL) ||
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        state |= Rml::Input::KM_CTRL;
    if ((mods & GLFW_MOD_SHIFT) ||
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        state |= Rml::Input::KM_SHIFT;
    if ((mods & GLFW_MOD_ALT) ||
        glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
        state |= Rml::Input::KM_ALT;
    if ((mods & GLFW_MOD_SUPER) ||
        glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
        state |= Rml::Input::KM_META;
    if (mods & GLFW_MOD_CAPS_LOCK) state |= Rml::Input::KM_CAPSLOCK;
    if (mods & GLFW_MOD_NUM_LOCK) state |= Rml::Input::KM_NUMLOCK;
    return state;
}

static Rml::Input::KeyIdentifier translate_key(int key) {
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        return (Rml::Input::KeyIdentifier)(Rml::Input::KI_A + key - GLFW_KEY_A);
    }
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        return (Rml::Input::KeyIdentifier)(Rml::Input::KI_0 + key - GLFW_KEY_0);
    }
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F24) {
        return (Rml::Input::KeyIdentifier)(Rml::Input::KI_F1 + key -
                                           GLFW_KEY_F1);
    }
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
        return (Rml::Input::KeyIdentifier)(Rml::Input::KI_NUMPAD0 + key -
                                           GLFW_KEY_KP_0);
    }

    switch (key) {
        case GLFW_KEY_SPACE:
            return Rml::Input::KI_SPACE;
        case GLFW_KEY_APOSTROPHE:
            return Rml::Input::KI_OEM_7;
        case GLFW_KEY_COMMA:
            return Rml::Input::KI_OEM_COMMA;
        case GLFW_KEY_MINUS:
            return Rml::Input::KI_OEM_MINUS;
        case GLFW_KEY_PERIOD:
            return Rml::Input::KI_OEM_PERIOD;
        case GLFW_KEY_SLASH:
            return Rml::Input::KI_OEM_2;
        case GLFW_KEY_SEMICOLON:
            return Rml::Input::KI_OEM_1;
        case GLFW_KEY_EQUAL:
            return Rml::Input::KI_OEM_PLUS;
        case GLFW_KEY_LEFT_BRACKET:
            return Rml::Input::KI_OEM_4;
        case GLFW_KEY_BACKSLASH:
            return Rml::Input::KI_OEM_5;
        case GLFW_KEY_RIGHT_BRACKET:
            return Rml::Input::KI_OEM_6;
        case GLFW_KEY_GRAVE_ACCENT:
            return Rml::Input::KI_OEM_3;
        case GLFW_KEY_ESCAPE:
            return Rml::Input::KI_ESCAPE;
        case GLFW_KEY_ENTER:
            return Rml::Input::KI_RETURN;
        case GLFW_KEY_TAB:
            return Rml::Input::KI_TAB;
        case GLFW_KEY_BACKSPACE:
            return Rml::Input::KI_BACK;
        case GLFW_KEY_INSERT:
            return Rml::Input::KI_INSERT;
        case GLFW_KEY_DELETE:
            return Rml::Input::KI_DELETE;
        case GLFW_KEY_RIGHT:
            return Rml::Input::KI_RIGHT;
        case GLFW_KEY_LEFT:
            return Rml::Input::KI_LEFT;
        case GLFW_KEY_DOWN:
            return Rml::Input::KI_DOWN;
        case GLFW_KEY_UP:
            return Rml::Input::KI_UP;
        case GLFW_KEY_PAGE_UP:
            return Rml::Input::KI_PRIOR;
        case GLFW_KEY_PAGE_DOWN:
            return Rml::Input::KI_NEXT;
        case GLFW_KEY_HOME:
            return Rml::Input::KI_HOME;
        case GLFW_KEY_END:
            return Rml::Input::KI_END;
        case GLFW_KEY_CAPS_LOCK:
            return Rml::Input::KI_CAPITAL;
        case GLFW_KEY_SCROLL_LOCK:
            return Rml::Input::KI_SCROLL;
        case GLFW_KEY_NUM_LOCK:
            return Rml::Input::KI_NUMLOCK;
        case GLFW_KEY_PRINT_SCREEN:
            return Rml::Input::KI_SNAPSHOT;
        case GLFW_KEY_PAUSE:
            return Rml::Input::KI_PAUSE;
        case GLFW_KEY_KP_DECIMAL:
            return Rml::Input::KI_DECIMAL;
        case GLFW_KEY_KP_DIVIDE:
            return Rml::Input::KI_DIVIDE;
        case GLFW_KEY_KP_MULTIPLY:
            return Rml::Input::KI_MULTIPLY;
        case GLFW_KEY_KP_SUBTRACT:
            return Rml::Input::KI_SUBTRACT;
        case GLFW_KEY_KP_ADD:
            return Rml::Input::KI_ADD;
        case GLFW_KEY_KP_ENTER:
            return Rml::Input::KI_NUMPADENTER;
        case GLFW_KEY_LEFT_SHIFT:
            return Rml::Input::KI_LSHIFT;
        case GLFW_KEY_LEFT_CONTROL:
            return Rml::Input::KI_LCONTROL;
        case GLFW_KEY_LEFT_ALT:
            return Rml::Input::KI_LMENU;
        case GLFW_KEY_LEFT_SUPER:
            return Rml::Input::KI_LMETA;
        case GLFW_KEY_RIGHT_SHIFT:
            return Rml::Input::KI_RSHIFT;
        case GLFW_KEY_RIGHT_CONTROL:
            return Rml::Input::KI_RCONTROL;
        case GLFW_KEY_RIGHT_ALT:
            return Rml::Input::KI_RMENU;
        case GLFW_KEY_RIGHT_SUPER:
            return Rml::Input::KI_RMETA;
        default:
            return Rml::Input::KI_UNKNOWN;
    }
}

void InputHandler::init(Window *window) {
    this->window = window;
    input_window = window;
    spdlog::info("Initializing Input handler");
    if (!window) {
        SPDLOG_ERROR(
            "Can't initialize Input handler, window is null, exiting.");
        exit(1);
    }

    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();
    glfwSetCharCallback(glfw_window, [](GLFWwindow *, unsigned int codepoint) {
        Rml::Context *context = System::gGuiEngine->get_rml_context();
        if (!context) return;
        context->ProcessTextInput((Rml::Character)codepoint);
    });
    glfwSetKeyCallback(glfw_window, [](GLFWwindow *window, int key,
                                       int scancode, int action, int mods) {
        KeyCode k = static_cast<KeyCode>(key);
        if (action == GLFW_PRESS) {
            System::gInput->key_pressed.insert(k);
        } else if (action == GLFW_RELEASE) {
            System::gInput->key_pressed.erase(k);
        }
        Rml::Context *context = System::gGuiEngine->get_rml_context();
        if (!context) return;
        Rml::Input::KeyIdentifier rml_key = translate_key(key);
        if (rml_key == Rml::Input::KI_UNKNOWN) return;
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            context->ProcessKeyDown(rml_key, modifier_state(window, mods));
        if (action == GLFW_RELEASE)
            context->ProcessKeyUp(rml_key, modifier_state(window, mods));
    });
    glfwSetCursorPosCallback(glfw_window, [](GLFWwindow *window, double x,
                                             double y) {
        Input *input = System::gInput;
        i32 ww, wh;
        Rml::Context *context = System::gGuiEngine->get_rml_context();
        if (context) {
            context->ProcessMouseMove((int)x, (int)y, modifier_state(window));
        }
        glfwGetWindowSize(window, &ww, &wh);
        x /= (f32)ww;
        y /= (f32)wh;
        if (input->drag_func) {
            input->drag_func(input->last_x, input->last_y, x, y);
        }

        input->last_x = x;
        input->last_y = y;
    });
    glfwSetMouseButtonCallback(
        glfw_window, [](GLFWwindow *window, int button, int action, int mods) {
            Input *input = System::gInput;
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
            } else if (action == GLFW_RELEASE) {
                input->mouse_pressed.erase(me);
            }
            Rml::Context *context = System::gGuiEngine->get_rml_context();
            if (!context) return;
            int rml_button = -1;
            if (button == GLFW_MOUSE_BUTTON_LEFT) rml_button = 0;
            if (button == GLFW_MOUSE_BUTTON_RIGHT) rml_button = 1;
            if (button == GLFW_MOUSE_BUTTON_MIDDLE) rml_button = 2;
            if (rml_button < 0) return;
            if (action == GLFW_PRESS)
                context->ProcessMouseButtonDown(rml_button,
                                                modifier_state(window, mods));
            if (action == GLFW_RELEASE)
                context->ProcessMouseButtonUp(rml_button,
                                              modifier_state(window, mods));
        });

    glfwSetScrollCallback(
        glfw_window, [](GLFWwindow *window, double xoffset, double yoffset) {
            Rml::Context *context = System::gGuiEngine->get_rml_context();
            if (!context) return;
            context->ProcessMouseWheel(-(float)yoffset, modifier_state(window));
        });
}

void InputHandler::update() {
    Input *input = System::gInput;
    input->last_mouse_pressed = input->mouse_pressed;
    input->last_key_pressed = input->key_pressed;
}
}  // namespace Seed
