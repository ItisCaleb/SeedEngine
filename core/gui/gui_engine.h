#ifndef _SEED_GUI_ENGINE_H_
#define _SEED_GUI_ENGINE_H_
#include "core/window.h"
#include "core/gui/gui.h"
#include <vector>

namespace Seed {
class GuiEngine {
    private:
        inline static GuiEngine *instance = nullptr;
        std::vector<GUI *> guis;

    public:
        void update();
        static GuiEngine *get_instance() { return instance; }
        void add_gui(GUI *gui) { this->guis.push_back(gui); }
        void remove_gui(GUI *gui) {
            auto iter = std::find(guis.begin(), guis.end(), gui);
            guis.erase(iter);
        }
        GuiEngine(Window *window);
        ~GuiEngine() { instance = nullptr; }
};
}  // namespace Seed

#endif