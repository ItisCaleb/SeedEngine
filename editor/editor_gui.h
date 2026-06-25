#ifndef _SEED_EDITOR_GUI_H_
#define _SEED_EDITOR_GUI_H_

#include <string>
#include "core/gui/gui.h"
#include "core/io/file.h"
#include "core/types.h"

namespace Seed {

class EditorGUI : public GUI {
    private:
        bool create_new_project = false;
        std::string project_name_input;
        std::string project_path_input;
        std::string project_error;
        std::string shader_reload_message;
        f32 main_menu_height = 0.0f;

        void *font = nullptr;
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
