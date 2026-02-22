#ifndef _SEED_EDITOR_INPUT_GUI_H_
#define _SEED_EDITOR_INPUT_GUI_H_

#include "core/gui/gui_engine.h"
#include "core/input.h"
#include <imgui.h>
#include <string>

namespace Seed {

class InputGUI : public GUI {
public:
    InputGUI() = default;
    virtual ~InputGUI() = default;

    void update() override;

private:
    std::string waiting_for_action = "";
    
    std::string key_to_string(KeyCode code);
};

} // namespace Seed

#endif