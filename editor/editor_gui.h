#ifndef _SEED_EDITOR_GUI_H_
#define _SEED_EDITOR_GUI_H_

#include "core/gui/gui.h"
#include "core/io/file.h"

namespace Seed {

class EditorGUI : public GUI {
    private:
        bool create_new_project = false;
        std::string project_name_input;
        std::string project_path_input;
        std::string project_error;
        float main_menu_height;


        void *font;
        void main_menu();
        void main_panel();

        /* popup */
        void create_project();

    public:
        void update() override;
        EditorGUI();
        ~EditorGUI();
};
}  // namespace Seed

#endif