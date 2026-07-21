#ifndef _SEED_RML_WIDGETS_H_
#define _SEED_RML_WIDGETS_H_
#include "core/types.h"
#include "core/misc/uuid.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>

namespace Seed {

#define RML_POPUP_NAME "x-popup"
#define RML_MENU_NAME "x-menu"
#define RML_MENU_ITEM_NAME "x-menu-item"

class RmlPopup : public Rml::Element, public Rml::EventListener {
    private:
        bool opened = false;

    public:
        void open(i32 x, i32 y);
        virtual void close();
        void ProcessEvent(Rml::Event &event) override;

        RmlPopup(const Rml::String &tag);
        ~RmlPopup() { close(); }
};

class RmlMenuItem : public Rml::Element {
    private:
        Rml::Context *context = nullptr;

    public:
        void ProcessDefaultAction(Rml::Event &event) override;

        RmlMenuItem(const Rml::String &tag);
        ~RmlMenuItem() = default;
};

class RmlMenu : public Rml::Element, public Rml::EventListener {
        friend RmlMenuItem;

    private:
        bool opened = false;
        bool is_popup;
        RmlMenu *submenu = nullptr;
        Rml::Element *event_target = nullptr;
        bool is_root();

    public:
        void open(i32 x, i32 y);
        void close();
        void ProcessEvent(Rml::Event &event) override;
        void OnDetach(Rml::Element *element) override;

        RmlMenu(const Rml::String &tag);
        ~RmlMenu() { close(); }
};
}  // namespace Seed

#endif
