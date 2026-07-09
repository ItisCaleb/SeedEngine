#include "editor_ui.h"
#include <algorithm>

namespace Seed {
namespace EditorUI {

ScopedID::ScopedID(const char *id) { ImGui::PushID(id); }

ScopedID::ScopedID(i32 id) { ImGui::PushID((i32)id); }

ScopedID::~ScopedID() { ImGui::PopID(); }

ScopedGroup::ScopedGroup() { ImGui::BeginGroup(); }

ScopedGroup::~ScopedGroup() { ImGui::EndGroup(); }

DisabledScope::DisabledScope(bool disabled) : disabled(disabled) {
    if (disabled) {
        ImGui::BeginDisabled();
    }
}

DisabledScope::~DisabledScope() {
    if (disabled) {
        ImGui::EndDisabled();
    }
}

ScopedStyleColor::ScopedStyleColor(ImGuiCol idx, const ImVec4 &color) {
    ImGui::PushStyleColor(idx, color);
}

ScopedStyleColor::~ScopedStyleColor() { ImGui::PopStyleColor(); }

ScopedStyleVar::ScopedStyleVar(ImGuiStyleVar idx, f32 value) {
    ImGui::PushStyleVar(idx, value);
}

ScopedStyleVar::ScopedStyleVar(ImGuiStyleVar idx, const ImVec2 &value) {
    ImGui::PushStyleVar(idx, value);
}

ScopedStyleVar::~ScopedStyleVar() { ImGui::PopStyleVar(); }

ScopedClipRect::ScopedClipRect(ImVec2 min, ImVec2 max,
                               bool intersect_with_current) {
    ImGui::PushClipRect(min, max, intersect_with_current);
}

ScopedClipRect::~ScopedClipRect() { ImGui::PopClipRect(); }

void section(const char *label) {
    ImGui::Spacing();
    ImGui::TextUnformatted(label);
    ImGui::Separator();
    ImGui::Spacing();
}

static bool draw_splitter(const char *id, bool vertical, f32 length,
                          f32 thickness, f32 &value, f32 min_value,
                          f32 max_value) {
    if (max_value < min_value) max_value = min_value;
    if(length == 0.0) length = 1.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size =
        vertical ? ImVec2(thickness, length) : ImVec2(length, thickness);
    ImGui::InvisibleButton(id, size);

    bool active = ImGui::IsItemActive();
    if (active) {
        f32 delta = vertical ? ImGui::GetIO().MouseDelta.x
                             : ImGui::GetIO().MouseDelta.y;
        value = std::clamp(value + delta, min_value, max_value);
    }

    if (ImGui::IsItemHovered() || active) {
        ImGui::SetMouseCursor(vertical ? ImGuiMouseCursor_ResizeEW
                                       : ImGuiMouseCursor_ResizeNS);
    }

    ImGui::GetWindowDrawList()->AddRectFilled(
        pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(55, 55, 55, 255));
    return active;
}

bool horizontal_splitter(const char *id, f32 width, f32 thickness, f32 &height,
                         f32 min_height, f32 max_height) {
    return draw_splitter(id, false, width, thickness, height, min_height,
                         max_height);
}

bool vertical_splitter(const char *id, f32 height, f32 thickness, f32 &width,
                       f32 min_width, f32 max_width) {
    return draw_splitter(id, true, height, thickness, width, min_width,
                         max_width);
}

bool draw_vec3(KStr label, Vec3 &value, f32 speed) {
    ScopedID id(label.data());
    ImGui::TextUnformatted(label.data(), label.end());
    bool changed = ImGui::DragFloat3("##value", value.coord, speed);
    return changed;
}

void draw_checkerboard(ImDrawList *draw_list, ImVec2 min, ImVec2 max,
                       f32 cell_size) {
    const ImU32 a = IM_COL32(54, 57, 63, 255);
    const ImU32 b = IM_COL32(70, 74, 82, 255);
    for (f32 y = min.y; y < max.y; y += cell_size) {
        for (f32 x = min.x; x < max.x; x += cell_size) {
            i32 ix = (i32)((x - min.x) / cell_size);
            i32 iy = (i32)((y - min.y) / cell_size);
            ImU32 color = ((ix + iy) & 1) == 0 ? a : b;
            draw_list->AddRectFilled(ImVec2(x, y),
                                     ImVec2(std::min(x + cell_size, max.x),
                                            std::min(y + cell_size, max.y)),
                                     color);
        }
    }
}

void draw_centered_text(ImDrawList *draw_list, const char *text, ImVec2 min,
                        ImVec2 max, ImU32 color) {
    ImVec2 text_size = ImGui::CalcTextSize(text);
    draw_list->AddText(ImVec2(min.x + (max.x - min.x - text_size.x) * 0.5f,
                              min.y + (max.y - min.y - text_size.y) * 0.5f),
                       color, text);
}

void draw_centered_wrapped_text(KStr text, ImVec2 min, ImVec2 max,
                                i32 max_lines) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const char *text_begin = text.data();
    const char *text_end = text.end();
    const f32 line_height = ImGui::GetTextLineHeight();
    const f32 wrap_width = max.x - min.x;
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);

    const char *line_begin = text_begin;
    for (i32 line = 0; line < max_lines && line_begin < text_end; line++) {
        while (line_begin < text_end &&
               (*line_begin == ' ' || *line_begin == '\n' ||
                *line_begin == '\r')) {
            line_begin++;
        }
        if (line_begin >= text_end) break;

        const char *line_end = line_begin;
        const char *best_fit = line_begin;
        const char *best_separator_break = nullptr;
        for (const char *cursor = line_begin; cursor < text_end;) {
            const char *next = cursor + 1;
            while (next < text_end && ((*next & 0xC0) == 0x80)) next++;

            ImVec2 text_size = ImGui::CalcTextSize(line_begin, next);
            if (text_size.x > wrap_width && best_fit > line_begin) break;

            best_fit = next;
            bool is_separator = *cursor == '_' || *cursor == '-' ||
                                *cursor == ' ' || *cursor == '.';
            bool is_leading_dot = cursor == text_begin && *cursor == '.';
            if (is_separator && !is_leading_dot) {
                const char *candidate = *cursor == '.' ? cursor : next;
                if (candidate > line_begin) best_separator_break = candidate;
            }

            cursor = next;
        }

        line_end = best_fit;
        if (best_separator_break != nullptr &&
            best_separator_break > line_begin) {
            line_end = best_separator_break;
        }
        if (line_end <= line_begin) {
            line_end = line_begin + 1;
            while (line_end < text_end && ((*line_end & 0xC0) == 0x80))
                line_end++;
        }

        const char *draw_end = line_end;
        while (draw_end > line_begin &&
               (draw_end[-1] == ' ' || draw_end[-1] == '\n' ||
                draw_end[-1] == '\r')) {
            draw_end--;
        }

        ImVec2 text_size = ImGui::CalcTextSize(line_begin, draw_end);
        f32 x = min.x + std::max(0.f, (wrap_width - text_size.x) * 0.5f);
        f32 y = min.y + line * line_height;
        if (y + line_height > max.y) break;

        draw_list->AddText(ImVec2(x, y), color, line_begin, draw_end);
        line_begin = line_end;
    }
}

void draw_empty_panel(ImDrawList *draw_list, ImVec2 min, ImVec2 max,
                      const char *message) {
    draw_list->AddRectFilled(min, max, IM_COL32(20, 22, 26, 255));
    draw_centered_text(draw_list, message, min, max,
                       IM_COL32(90, 96, 108, 255));
}

void draw_texture_preview(const TexturePreview *preview, ImVec2 min, ImVec2 max,
                          bool selected) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_checkerboard(draw_list, min, max);

    if (preview != nullptr && !preview->texture.is_null()) {
        f32 box_w = max.x - min.x;
        f32 box_h = max.y - min.y;
        f32 image_w = (f32)std::max(1u, preview->width);
        f32 image_h = (f32)std::max(1u, preview->height);
        f32 scale = std::min(box_w / image_w, box_h / image_h);
        ImVec2 size(image_w * scale, image_h * scale);
        ImVec2 image_min(min.x + (box_w - size.x) * 0.5f,
                         min.y + (box_h - size.y) * 0.5f);
        ImVec2 image_max(image_min.x + size.x, image_min.y + size.y);
        draw_list->AddImage((ImTextureID)(u64)preview->texture->get_handle(),
                            image_min, image_max);
    } else {
        draw_centered_text(
            draw_list,
            preview != nullptr && preview->failed ? "Failed" : "Empty", min,
            max, IM_COL32(185, 190, 200, 255));
    }

    ImU32 border = IM_COL32(105, 110, 120, 255);
    if (preview != nullptr && preview->upload_failed) {
        border = IM_COL32(230, 165, 80, 255);
    }
    if (selected) {
        border = IM_COL32(110, 180, 255, 255);
    }
    draw_list->AddRect(min, max, border, 4.0f, 0, selected ? 2.0f : 1.0f);
}

UUID accept_uuid() {
    UUID uuid{};
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("UUID")) {
            uuid = *(UUID *)payload->Data;
        }
        ImGui::EndDragDropTarget();
    }
    return uuid;
}

}  // namespace EditorUI
}  // namespace Seed
