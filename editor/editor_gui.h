#ifndef _SEED_EDITOR_GUI_H_
#define _SEED_EDITOR_GUI_H_
#include "core/gui/gui.h"
#include "core/io/file.h"

namespace Seed {

class EditorGUI : public GUI {
    private:
        void *font;
        void main_menu();
        void editor_left_panel();
        void character_database();

        /* popup */
        void create_project();

    public:
        void update() override;
        EditorGUI();
};
}  // namespace Seed

#endif