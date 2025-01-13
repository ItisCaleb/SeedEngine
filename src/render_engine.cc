#include <seed/render_engine.h>
#include <spdlog/spdlog.h>

namespace Seed {
RenderEngine *RenderEngine::get_instance() { return instance; }
RenderEngine::RenderEngine(GLFWwindow *window, int w, int h) {
    instance = this;
    spdlog::info("Initializing Rendering engine");

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::error("Can't initialize GLAD. Exiting");
        exit(1);
    }
    glViewport(0, 0, w, h);
}
RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed