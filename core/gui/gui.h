#ifndef _SEED_GUI_H_
#define _SEED_GUI_H_
#include <imgui.h>

namespace Seed {

class GUI {
    public:
        virtual void update() = 0;
        GUI() = default;
};
}  // namespace Seed

#endif