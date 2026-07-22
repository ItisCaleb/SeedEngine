#include "gui.h"
#include "core/gui/gui_engine.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/Factory.h>
#include "core/resource/resource_loader.h"

namespace Seed {

void RmlGUI::load_document(Ref<GuiDocument> doc) {
    if (doc.is_null()) return;
    Rml::Context *context = System::gGuiEngine->get_rml_context();
    if (!context) return;
    if (this->document) {
        document->Close();
    }

    Rml::StreamMemory stream((const Rml::byte *)doc->content.data(),
                             doc->content.size());
    stream.SetSourceURL(doc->source.data());
    this->document = context->LoadDocument(&stream);
}

void RmlGUI::init() {
    if (!first_init) {
        bind_model(System::gGuiEngine->get_rml_context());
        first_init = true;
    }
    load_document(this->original_document);
    auto set_document = [](auto &&self, RmlGUI *gui,
                           Rml::ElementDocument *doc) -> void {
        for (RmlGUI *child : gui->childs) {
            child->document = doc;
            self(self, child, doc);
        }
    };
    set_document(set_document, this, this->document);
}

void RmlGUI::open_gui(RmlGUI *gui) {
    gui->document = document;
    gui->init();
    childs.push_back(gui);
}

bool RmlGUI::reload() {
    if (original_document.is_null()) return false;

    Ref<GuiDocument> doc = System::gResourceLoader->load_internal<GuiDocument>(
        original_document->source);
    if (doc.is_null()) return false;
    Rml::Factory::ClearTemplateCache();
    Rml::Factory::ClearStyleSheetCache();
    this->original_document = doc;
    init();
    load_document(this->original_document);

    auto set_document = [](auto &&self, RmlGUI *gui,
                           Rml::ElementDocument *doc) -> void {
        for (RmlGUI *child : gui->childs) {
            child->document = doc;
            self(self, child, doc);
        }
    };
    set_document(set_document, this, this->document);

    this->document->Show();
    return true;
}

RmlGUI::~RmlGUI() {
    if (original_document.is_valid() && document) document->Close();
}

}  // namespace Seed