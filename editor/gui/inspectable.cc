#include "inspectable.h"
#include <imgui.h>
#include "core/container/kstring.h"
#include "core/misc/uuid.h"
#include "editor/editor.h"

namespace Seed {

bool Inspectable::drag_uuid(KStr name, UUID &uuid) {
    ImGui::TextUnformatted(name.data(), name.end());
    ImGui::SameLine();
    KString uuid_text = uuid.to_string();
    ImGui::TextUnformatted(uuid_text.data());
    if (ImGui::BeginDragDropTarget()) {
        if (auto p = ImGui::AcceptDragDropPayload("UUID")) {
            uuid = *(UUID *)p->Data;
            return true;
        }
    }
    return false;
}

void Inspector::update() {
    if (gEditor->ctx.current_inspect == nullptr) return;
    gEditor->ctx.current_inspect->draw_inspector();
}
}  // namespace Seed