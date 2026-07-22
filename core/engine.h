#ifndef _SEED_ENGINE_H_
#define _SEED_ENGINE_H_

#include "core/io/path.h"
#include "core/types.h"
#include "core/input_handler.h"

namespace Seed {

class Project;
class Window;
class World;

struct EngineConfig {
        enum DebugFlag : u8 {
            NONE,
            BOUNDING_BOX,
            FRUSTUM,
            MODEL_NORMAL,
            PHYSIC
        };
        DebugFlag debug_flag = DebugFlag::NONE;
};

class SeedEngine {
    private:
        Project *current_project = nullptr;
        InputHandler input_handler;
        f32 frame_limit = 60.0;
        Window *window;
        World *world;
        f32 last_fps;
        EngineConfig config;
        void setup_logger();
        void init_systems();
        void deinit_systems();

    public:
        int width, height;
        void start();
        Project *get_project() { return current_project; }
        World *get_world() { return world; }
        Window *get_window() { return window; }
        f32 get_fps() { return last_fps; }
        void set_debug_flag(EngineConfig::DebugFlag flag) {
            this->config.debug_flag = flag;
        }
        EngineConfig::DebugFlag get_debug_flag() const {
            return this->config.debug_flag;
        }

        bool load_project(const Path &path);
        SeedEngine(f32 target_fps = 60.0);
        ~SeedEngine();

        inline void set_fps(f32 target_fps) {
            this->frame_limit = 1 / target_fps;
        }
};
}  // namespace Seed

#endif
