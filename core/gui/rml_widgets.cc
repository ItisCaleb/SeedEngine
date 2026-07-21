#include "rml_widgets.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <fmt/format.h>

namespace Seed {

static bool element_contains(Rml::Element *target, Rml::Element *element) {
    while (element != nullptr) {
        if (element == target) return true;
        element = element->GetParentNode();
    }
    return false;
}

RmlPopup::RmlPopup(const Rml::String &tag) : Rml::Element(tag) {
    SetProperty("position", "absolute");
    SetProperty("display", "none");
    SetProperty("min-width", "150dp");
    SetProperty("padding", "6dp 0dp");
    SetProperty("color", "#d7dde7");
    SetProperty("background-color", "#20242b");
    SetProperty("border", "1dp #4a5564");
}
void RmlPopup::open(i32 x, i32 y) {
    SetProperty("display", "block");
    SetProperty("left", fmt::format("{}px", x));
    SetProperty("top", fmt::format("{}px", y));

    if (!opened) {
        AddEventListener("mousedown", this, true);
        AddEventListener("keydown", this, true);
    }
    opened = true;
}
void RmlPopup::close() {
    SetProperty("display", "none");

    if (opened) {
        RemoveEventListener("mousedown", this, true);
        RemoveEventListener("keydown", this, true);
    }
    opened = false;
}

void RmlPopup::ProcessEvent(Rml::Event &event) {
    if (event.GetType() == "mousedown") {
        if (!element_contains(this, event.GetTargetElement())) close();
    }
}

RmlMenu::RmlMenu(const Rml::String &tag) : Rml::Element(tag) {
    SetProperty("position", "absolute");
    SetProperty("display", "none");
    SetProperty("padding", "0dp");
    SetProperty("white-space", "nowrap");
    SetProperty("background-color", "#20242b");
    SetProperty("border", "1dp #4a5564");
}

bool RmlMenu::is_root() {
    Rml::Element *element = this->GetParentNode();
    while (element) {
        if (element->GetTagName() == RML_MENU_NAME) {
            return false;
        }
        element = element->GetParentNode();
    }
    return true;
}

void RmlMenu::open(i32 x, i32 y) {
    SetProperty("display", "block");
    SetProperty("left", fmt::format("{}px", x));
    SetProperty("top", fmt::format("{}px", y));

    if (is_root() && !opened) {
        event_target = GetOwnerDocument();
        if (event_target != nullptr) {
            event_target->AddEventListener("mousedown", this, true);
            event_target->AddEventListener("keydown", this, true);
        }
    }
    opened = true;
}

void RmlMenu::close() {
    SetProperty("display", "none");

    if (event_target != nullptr) {
        Rml::Element *target = event_target;
        target->RemoveEventListener("mousedown", this, true);
        target->RemoveEventListener("keydown", this, true);
        event_target = nullptr;
    }
    if (this->submenu) {
        this->submenu->close();
        this->submenu = nullptr;
    }
    opened = false;
}

/* When hovered to menu item, try open its first submenu*/
/* All menu will be closed when clicking outside it. */
void RmlMenu::ProcessEvent(Rml::Event &event) {
    if (event.GetType() == "mousedown") {
        if (!element_contains(this, event.GetTargetElement())) close();
    }
}

void RmlMenu::OnDetach(Rml::Element *element) {
    if (event_target == element) {
        event_target = nullptr;
    }
}

RmlMenuItem::RmlMenuItem(const Rml::String &tag) : Rml::Element(tag) {
    SetProperty("display", "block");
    SetProperty("position", "relative");
    SetProperty("min-width", "80dp");
    SetProperty("line-height", "18dp");
    SetProperty("padding", "5dp 28dp 5dp 12dp");
    SetProperty("white-space", "nowrap");
    SetProperty("color", "#d7dde7");
    SetProperty("background-color", "transparent");
}

void RmlMenuItem::ProcessDefaultAction(Rml::Event &event) {
    /* when hovered to this element */
    if (event.GetPhase() == Rml::EventPhase::Target &&
        event.GetType() == "mouseover") {
        SetProperty("background-color", "#34516f");

        /* try open sub menu */
        Rml::ElementList list;
        this->GetElementsByTagName(list, RML_MENU_NAME);
        if (list.size() >= 1) {
            RmlMenu *child_menu = dynamic_cast<RmlMenu *>(list[0]);
            RmlMenu *parent_menu = dynamic_cast<RmlMenu *>(GetParentNode());
            if (!child_menu || !parent_menu) return;

            Rml::Vector2f offset = GetAbsoluteOffset();
            Rml::Vector2f size = GetRenderBox().GetFillSize();
            if (parent_menu->submenu) parent_menu->submenu->close();
            child_menu->open(size.x, -5);
            parent_menu->submenu = child_menu;
        }
    } else if (event.GetPhase() == Rml::EventPhase::Target &&
               event.GetType() == "mouseout") {
        SetProperty("background-color", "transparent");
    }
}
}  // namespace Seed
