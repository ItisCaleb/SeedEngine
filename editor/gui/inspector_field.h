#ifndef _SEED_INSPECTOR_FIELD_H_
#define _SEED_INSPECTOR_FIELD_H_

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>

#include "core/container/kstring.h"
#include "core/types.h"

namespace Seed {

#define RML_INSPECTOR_FIELD "x-inspector-field"

class RmlInspectorField : public Rml::Element, public Rml::EventListener {
    private:
        i32 section_index = -1;
        i32 field_index = -1;

        Rml::String encode(KStr value) const;
        void rebuild();
        void dispatch_change(Rml::Event &event);

    protected:
        void OnAttributeChange(
            const Rml::ElementAttributes &changed_attributes) override;

    public:
        explicit RmlInspectorField(const Rml::String &tag);
        ~RmlInspectorField();
        void ProcessEvent(Rml::Event &event) override;
};

}  // namespace Seed

#endif
