#include "window.h"
#include <GLFW/glfw3.h>
#include "core/engine.h"

namespace Seed {

static void on_window_resize(GLFWwindow *window, i32 w, i32 h) {
    
}

Window::Window(u32 w, u32 h, const std::string &title)
    : w(w), h(h), title(title) {
    this->window = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);
    glfwSetWindowUserPointer((GLFWwindow *)this->window,
                             static_cast<void *>(this));

    glfwGetFramebufferSize((GLFWwindow *)this->window, (i32 *)&this->w,
                           (i32 *)&this->h);

    glfwSetFramebufferSizeCallback(
        (GLFWwindow *)this->window, [](GLFWwindow *, i32 w, i32 h) {
            Window *window = SeedEngine::get_instance()->get_window();
            window->w = w;
            window->h = h;
        });
}

Window::~Window() { glfwDestroyWindow(get_window<GLFWwindow>()); }
}  // namespace Seed