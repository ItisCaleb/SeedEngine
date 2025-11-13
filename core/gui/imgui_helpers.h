#ifndef _SEED_IMGUI_HELPER_H_
#define _SEED_IMGUI_HELPER_H_
#include <functional>

#include <imgui.h>

// Defines a “tab”: icon glyph, optional tooltip, and a draw callback
struct VerticalIconTab {
        const char *icon;            // Font Awesome glyph
        const char *tooltip;         // Shown on hover (optional)
        std::function<void()> draw;  // Lambda that draws the page
};

void drawRVerticalIconTabs(const VerticalIconTab *tabs, int tabCount,
                           int &currentTab) {
    // 1) sizes
    const float pageWidth = 300.f;  // fixed page width
    const float barWidth = 38.f;    // icon strip width
    const float btnSize = 26.f;     // square icon buttons
    const ImVec2 iconSize{btnSize, btnSize};

    // 2) tighten horizontal spacing
    ImGuiStyle &style = ImGui::GetStyle();
    float savedSpacingX = style.ItemSpacing.x;
    style.ItemSpacing.x = 4.f;  // gap between page & bar

    // A) LEFT: only draw page if a tab is active
    if (currentTab >= 0) {
        ImGui::BeginChild("##page", ImVec2(pageWidth, 0), false,
                          ImGuiWindowFlags_NoScrollWithMouse);
        tabs[currentTab].draw();
        ImGui::EndChild();
        ImGui::SameLine();
    }

    // B) RIGHT: always-visible icon strip
    ImGui::BeginChild(
        "##icon_bar", ImVec2(barWidth, 0), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    for (int i = 0; i < tabCount; ++i) {
        ImGui::PushID(i);
        bool sel = (currentTab == i);

        // highlight selected tab
        if (sel)
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.30f, 0.44f, 0.60f, 1.f)),
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                      ImVec4(0.32f, 0.49f, 0.68f, 1.f)),
                ImGui::PushStyleColor(ImGuiCol_Border,
                                      ImVec4(0.80f, 0.80f, 0.90f, 1.f));

        // ImGui::PushFont(myIconFont);
        bool pressed = ImGui::Button(tabs[i].icon, iconSize);
        // ImGui::PopFont();

        if (pressed) currentTab = sel ? -1 : i;  // toggle off on re-click

        if (ImGui::IsItemHovered() && tabs[i].tooltip)
            ImGui::SetTooltip("%s", tabs[i].tooltip);

        if (sel) ImGui::PopStyleColor(3);

        ImGui::PopID();
    }

    ImGui::EndChild();
    style.ItemSpacing.x = savedSpacingX;
}

void drawLVerticalIconTabs(const VerticalIconTab *tabs, int tabCount,
                           int &currentTab) {
    // 1) sizes
    const float pageWidth = 300.f;  // fixed page width
    const float barWidth = 38.f;    // icon strip width
    const float btnSize = 26.f;     // square icon buttons
    const ImVec2 iconSize{btnSize, btnSize};

    // 2) tighten horizontal spacing
    ImGuiStyle &style = ImGui::GetStyle();
    float savedSpacingX = style.ItemSpacing.x;
    style.ItemSpacing.x = 0.f;  // gap between page & bar

    // B) RIGHT: always-visible icon strip
    ImGui::BeginChild(
        "##icon_bar", ImVec2(barWidth, 0), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    for (int i = 0; i < tabCount; ++i) {
        ImGui::PushID(i);
        bool sel = (currentTab == i);

        // highlight selected tab
        if (sel)
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.30f, 0.44f, 0.60f, 1.f)),
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                      ImVec4(0.32f, 0.49f, 0.68f, 1.f)),
                ImGui::PushStyleColor(ImGuiCol_Border,
                                      ImVec4(0.80f, 0.80f, 0.90f, 1.f));

        // ImGui::PushFont(myIconFont);
        bool pressed = ImGui::Button(tabs[i].icon, iconSize);
        // ImGui::PopFont();

        if (pressed) currentTab = sel ? -1 : i;  // toggle off on re-click

        if (ImGui::IsItemHovered() && tabs[i].tooltip)
            ImGui::SetTooltip("%s", tabs[i].tooltip);

        if (sel) ImGui::PopStyleColor(3);

        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::SameLine();

    // A) LEFT: only draw page if a tab is active
    if (currentTab >= 0) {
        ImGui::BeginChild("##page", ImVec2(pageWidth, 0), false,
                          ImGuiWindowFlags_NoScrollWithMouse);
        tabs[currentTab].draw();
        ImGui::EndChild();
    }
    style.ItemSpacing.x = savedSpacingX;
}

#endif