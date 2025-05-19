#ifndef _SEED_WINDOW_H_
#define _SEED_WINDOW_H_
#include "core/types.h"
#include <string>

namespace Seed {
class Window {
    private:
        u32 w, h;
        std::string title;
        bool fullscreen;
        void *window;
        void on_resize(void *window, i32 w, i32 h);

    public:
        u32 get_width() { return w; }
        u32 get_height() { return h; }

        template <typename T>
        T *get_window() {
            return static_cast<T *>(window);
        }
        Window(u32 w, u32 h, const std::string &title);
        ~Window();
};
}  // namespace Seed

#endif