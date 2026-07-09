#ifndef _SEED_EDITOR_UI_H_
#define _SEED_EDITOR_UI_H_

#include <imgui.h>
#include "core/container/kstring.h"
#include "core/math/vec3.h"
#include "core/ref.h"
#include "core/resource/texture.h"
#include "core/types.h"

namespace Seed {
namespace EditorUI {

struct TexturePreview {
        Ref<Texture> texture;
        u32 width = 0;
        u32 height = 0;
        bool failed = false;
        bool upload_failed = false;
};

class ScopedID {
    public:
        explicit ScopedID(const char *id);
        explicit ScopedID(i32 id);
        ~ScopedID();

        ScopedID(const ScopedID &) = delete;
        ScopedID &operator=(const ScopedID &) = delete;
};

class ScopedGroup {
    public:
        ScopedGroup();
        ~ScopedGroup();

        ScopedGroup(const ScopedGroup &) = delete;
        ScopedGroup &operator=(const ScopedGroup &) = delete;
};

class DisabledScope {
    private:
        bool disabled;

    public:
        explicit DisabledScope(bool disabled);
        ~DisabledScope();

        DisabledScope(const DisabledScope &) = delete;
        DisabledScope &operator=(const DisabledScope &) = delete;
};

class ScopedStyleColor {
    public:
        ScopedStyleColor(ImGuiCol idx, const ImVec4 &color);
        ~ScopedStyleColor();

        ScopedStyleColor(const ScopedStyleColor &) = delete;
        ScopedStyleColor &operator=(const ScopedStyleColor &) = delete;
};

class ScopedStyleVar {
    public:
        ScopedStyleVar(ImGuiStyleVar idx, f32 value);
        ScopedStyleVar(ImGuiStyleVar idx, const ImVec2 &value);
        ~ScopedStyleVar();

        ScopedStyleVar(const ScopedStyleVar &) = delete;
        ScopedStyleVar &operator=(const ScopedStyleVar &) = delete;
};

class ScopedClipRect {
    public:
        ScopedClipRect(ImVec2 min, ImVec2 max, bool intersect_with_current);
        ~ScopedClipRect();

        ScopedClipRect(const ScopedClipRect &) = delete;
        ScopedClipRect &operator=(const ScopedClipRect &) = delete;
};

void section(const char *label);
bool horizontal_splitter(const char *id, f32 width, f32 thickness, f32 &height,
                         f32 min_height, f32 max_height);
bool vertical_splitter(const char *id, f32 height, f32 thickness, f32 &width,
                       f32 min_width, f32 max_width);
bool draw_vec3(KStr label, Vec3 &value, f32 speed = 0.05f);
void draw_checkerboard(ImDrawList *draw_list, ImVec2 min, ImVec2 max,
                       f32 cell_size = 8.0f);
void draw_centered_text(ImDrawList *draw_list, const char *text, ImVec2 min,
                        ImVec2 max, ImU32 color);
void draw_centered_wrapped_text(KStr text, ImVec2 min, ImVec2 max,
                                i32 max_lines);
void draw_empty_panel(ImDrawList *draw_list, ImVec2 min, ImVec2 max,
                      const char *message);
void draw_texture_preview(const TexturePreview *preview, ImVec2 min, ImVec2 max,
                          bool selected);

UUID accept_uuid();

}  // namespace EditorUI
}  // namespace Seed

#endif
