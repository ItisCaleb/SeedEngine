#include "inspectable.h"
#include <imgui.h>
#include "core/container/kstring.h"
#include "core/misc/uuid.h"
#include "editor/editor.h"
#include "editor_ui.h"

namespace Seed {

bool Inspectable::drag_uuid(KStr name, UUID &uuid) {
    ImGui::TextUnformatted(name.data(), name.end());
    ImGui::SameLine();
    KString uuid_text = uuid.to_string();
    ImGui::TextUnformatted(uuid_text.data());
    UUID accepted = EditorUI::accept_uuid();
    if (accepted.is_null()) return false;

    uuid = accepted;
    return true;
}

void Inspector::update() {
    if (gEditor->ctx.current_inspect == nullptr) return;
    gEditor->ctx.current_inspect->draw_inspector();
}
}  // namespace Seed
