#ifndef _SEED_INPUT_H_
#define _SEED_INPUT_H_
#include <set>
#include "core/types.h"
#include "core/input_handler.h"

namespace Seed {

enum class KeyCode {
    SPACE = 0x0020,
    EXCLAM = 0x0021,
    QUOTEDBL = 0x0022,
    NUMBERSIGN = 0x0023,
    DOLLAR = 0x0024,
    PERCENT = 0x0025,
    AMPERSAND = 0x0026,
    APOSTROPHE = 0x0027,
    PARENLEFT = 0x0028,
    PARENRIGHT = 0x0029,
    ASTERISK = 0x002A,
    PLUS = 0x002B,
    COMMA = 0x002C,
    MINUS = 0x002D,
    PERIOD = 0x002E,
    SLASH = 0x002F,
    NUM_0 = 0x0030,
    NUM_1 = 0x0031,
    NUM_2 = 0x0032,
    NUM_3 = 0x0033,
    NUM_4 = 0x0034,
    NUM_5 = 0x0035,
    NUM_6 = 0x0036,
    NUM_7 = 0x0037,
    NUM_8 = 0x0038,
    NUM_9 = 0x0039,
    COLON = 0x003A,
    SEMICOLON = 0x003B,
    LESS = 0x003C,
    EQUAL = 0x003D,
    GREATER = 0x003E,
    QUESTION = 0x003F,
    A = 0x0041,
    B = 0x0042,
    C = 0x0043,
    D = 0x0044,
    E = 0x0045,
    F = 0x0046,
    G = 0x0047,
    H = 0x0048,
    I = 0x0049,
    J = 0x004A,
    K = 0x004B,
    L = 0x004C,
    M = 0x004D,
    N = 0x004E,
    O = 0x004F,
    P = 0x0050,
    Q = 0x0051,
    R = 0x0052,
    S = 0x0053,
    T = 0x0054,
    U = 0x0055,
    V = 0x0056,
    W = 0x0057,
    X = 0x0058,
    Y = 0x0059,
    Z = 0x005A,
    BRACKETLEFT = 0x005B,
    BACKSLASH = 0x005C,
    BRACKETRIGHT = 0x005D,
    ASCIICIRCUM = 0x005E,
    UNDERSCORE = 0x005F,
    QUOTELEFT = 0x0060
};

enum class MouseEvent { LEFT, MIDDLE, RIGHT };

class Input {
        friend InputHandler;

    private:
        inline static Input *instance = nullptr;
        std::set<KeyCode> key_pressed;
        std::set<MouseEvent> mouse_pressed;
        std::function<void(i32 last_x, i32 last_y, i32 x, i32 y)> drag_func;
        i32 last_x = 0, last_y = 0;
        bool should_capture_mouse = true;;

    public:
        static Input *get_instance();
        void reset_input();
        bool is_key_pressed(KeyCode code);
        void on_mouse_move(
            std::function<void(i32 last_x, i32 last_y, i32 x, i32 y)> cb);
        bool is_mouse_clicked(MouseEvent e);
        void mouse_click(MouseEvent e);
        void set_capture_mouse(bool on){
            should_capture_mouse = on;
        }
        Input();
        ~Input();
};

}  // namespace Seed
#endif