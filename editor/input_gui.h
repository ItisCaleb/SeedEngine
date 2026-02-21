#ifndef _SEED_EDITOR_INPUT_GUI_H_
#define _SEED_EDITOR_INPUT_GUI_H_

#include "core/gui/gui_engine.h"
#include "core/input.h"
#include <imgui.h>
#include <string>

namespace Seed {

// 繼承你的 GUI 類別 (假設你的基底類別是 GUI 或類似名稱，
// 從 TerrainGUI 的用法來看，它應該有個 virtual update 方法)
class InputGUI : public GUI {
public:
    InputGUI() = default;
    virtual ~InputGUI() = default;

    // 實作 GUI 繪製邏輯
    void update() override;

private:
    std::string waiting_for_action = "";
    
    // 輔助函式：將 KeyCode 轉為字串顯示
    std::string key_to_string(KeyCode code);
};

} // namespace Seed

#endif