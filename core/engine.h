#ifndef _SEED_ENGINE_H_
#define _SEED_ENGINE_H_

#include "types.h"
#include "world.h"
#include "input_handler.h"

namespace Seed {
class SeedEngine {
   private:
    inline static SeedEngine *instance = nullptr;
    InputHandler input_handler;
    f32 frame_limit = 60.0;
    void *window = nullptr;
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