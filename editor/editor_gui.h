#ifndef _SEED_EDITOR_GUI_H_
#define _SEED_EDITOR_GUI_H_
#include "core/gui/gui.h"

namespace Seed {

class EditorGUI : public GUI {
    private:
        void main_menu();
        void create_project();
    public:
        void update() override;
};
}  // namespace Seed

#endif