#include "window.h"
#include <GLFW/glfw3.h>

namespace Seed{
    Window::Window(u32 w, u32 h, const std::string &title):w(w), h(h), title(title){
       this->window = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);
       glfwGetFramebufferSize((GLFWwindow*)this->window, (i32*)&this->w, (i32*)&this->h);

    }

    Window::~Window(){
        glfwDestroyWindow(get_window<GLFWwindow>());
    }
}