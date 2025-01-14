#ifndef _SEED_ENGINE_H_
#define _SEED_ENGINE_H_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <seed/types.h>
#include <seed/world.h>

namespace Seed {
class SeedEngine {
   private:
    inline static SeedEngine *instance = nullptr;
    f32 frame_limit = 60.0;
    GLFWwindow *window = nullptr;
    void delay(f32 seconds);
    void init_systems();
    World *world;
   public:
    static SeedEngine *get_instance();
    int width, height;
    void start();
    World *get_world();
    SeedEngine(f32 target_fps = 60.0);
    ~SeedEngine();

    inline void set_fps(f32 target_fps) { this->frame_limit = 1 / target_fps; }
};
}  // namespace Seed

#endif