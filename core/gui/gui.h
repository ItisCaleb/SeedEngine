#ifndef _SEED_GUI_H_
#define _SEED_GUI_H_
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <vector>
#include "core/io/path.h"
#include "core/resource/resource.h"

namespace Seed {

class GuiEngine;
class ImGUI {
    public:
        virtual void update() = 0;
        ImGUI() = default;
};

class GuiDocument : public Resource {
    public:
        Path source;
        KString content;
};

#define RML_EVENT_ARGS \
    Rml::DataModelHandle model, Rml::Event &e, const Rml::VariantList &args

class RmlGUI {
        friend GuiEngine;

    private:
        bool first_init = false;
        virtual void bind_model(Rml::Context *context) {};
        std::vector<RmlGUI *> childs;

    protected:
        Ref<GuiDocument> original_document;
        Rml::ElementDocument *document = nullptr;
        void open_gui(RmlGUI *gui);
        void load_document(Ref<GuiDocument> doc);
        RmlGUI() = default;

    public:
        void init();
        bool reload();
        RmlGUI(Ref<GuiDocument> doc) : original_document(doc) {}
        virtual ~RmlGUI();
};
}  // namespace Seed

#endif
