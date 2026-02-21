#include "input_gui.h"
#include <fmt/format.h>

namespace Seed {

void InputGUI::update() {
    ImGui::Begin("Input Settings");

    auto input = Input::get_instance();
    if (!input) {
        ImGui::Text("Input instance not found.");
        ImGui::End();
        return;
    }

    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Click a button to rebind, then press any key.");
    ImGui::Separator();

    // 假設你在 input.h 中定義了 bindings 映射表 (std::map<std::string, KeyCode>)
    // 如果還沒定義，可以先在 Input 類別手動加幾個，例如 MoveUp, Jump 等
    for (auto& [action_name, bound_code] : input->bindings) {
        ImGui::Columns(2, "binding_columns", false);
        ImGui::SetColumnWidth(0, 150);

        ImGui::Text("%s", action_name.c_str());
        ImGui::NextColumn();

        std::string label = (waiting_for_action == action_name) 
                            ? "WAITING..." 
                            : key_to_string(bound_code);

        // 使用 PushID 避免多個按鈕 ID 衝突
        ImGui::PushID(action_name.c_str());
        if (ImGui::Button(label.c_str(), ImVec2(-1, 0))) {
            waiting_for_action = action_name;
        }
        ImGui::PopID();

        // 監聽按鍵
        if (waiting_for_action == action_name) {
            // 遍歷 KeyCode 範圍
            for (int i = (int)KeyCode::SPACE; i <= (int)KeyCode::QUOTELEFT; i++) {
                if (ImGui::IsKeyPressed((ImGuiKey)i)) {
                    bound_code = static_cast<KeyCode>(i);
                    waiting_for_action = ""; // 綁定成功
                    break;
                }
            }
            // Esc 取消
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                waiting_for_action = "";
            }
        }

        ImGui::NextColumn();
        ImGui::Columns(1);
    }

    if (ImGui::Button("Save Settings")) {
        // TODO: 實作將 input->bindings 序列化到 JSON 的功能
    }

    ImGui::End();
}

std::string InputGUI::key_to_string(KeyCode code) {
    // 你的 KeyCode 是 ASCII 格式，直接轉換即可
    char c = (char)code;
    if (c == ' ') return "SPACE";
    return std::string(1, c);
}

} // namespace Seed