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

    for (auto& [action_name, bound_code] : input->bindings) {
        ImGui::Columns(2, "binding_columns", false);
        ImGui::SetColumnWidth(0, 150);

        ImGui::Text("%s", action_name.c_str());
        ImGui::NextColumn();

        std::string label = (waiting_for_action == action_name) 
                            ? "WAITING..." 
                            : key_to_string(bound_code);

        ImGui::PushID(action_name.c_str());
        if (ImGui::Button(label.c_str(), ImVec2(-1, 0))) {
            waiting_for_action = action_name;
        }
        ImGui::PopID();

        if (waiting_for_action == action_name) {
            for (int i = (int)KeyCode::SPACE; i <= (int)KeyCode::QUOTELEFT; i++) {
                if (ImGui::IsKeyPressed((ImGuiKey)i)) {
                    bound_code = static_cast<KeyCode>(i);
                    waiting_for_action = "";
                    break;
                }
            }
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
    char c = (char)code;
    if (c == ' ') return "SPACE";
    return std::string(1, c);
}

} // namespace Seed