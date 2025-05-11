#ifndef _SEED_GUI_ENGINE_H_
#define _SEED_GUI_ENGINE_H_
#include "core/window.h"

namespace Seed {
class GuiEngine {
    private:
        inline static GuiEngine *instance = nullptr;

    public:
        void update();
        static GuiEngine *get_instance() { return instance; }
        GuiEngine(Window *window);
        ~GuiEngine() { instance = nullptr; }
};
}  // namespace Seed

#endif