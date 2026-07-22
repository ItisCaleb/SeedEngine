#ifndef _SEED_EDITOR_WIDGET_H_
#define _SEED_EDITOR_WIDGET_H_

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/EventListener.h>
#include "core/container/kstring.h"
#include "core/misc/uuid.h"

namespace Seed {

#define RML_UUID_DRAG "x-uuid-drag"
#define RML_UUID_DROP "x-uuid-drop"
#define RML_PREVIEW "x-preview"

class EditorRmlElementInstancer : public Rml::ElementInstancer {
    public:
        void RegisterElements();
        // Instances an element given the tag name and attributes.
        // @param[in] parent The element the new element is destined to be
        // parented to.
        // @param[in] tag The tag of the element to instance.
        // @param[in] attributes Dictionary of attributes.
        // @return A unique pointer to the instanced element.
        Rml::ElementPtr InstanceElement(
            Rml::Element *parent, const Rml::String &tag,
            const Rml::XMLAttributes &attributes) override;

        // Releases an element instanced by this instancer.
        // @param[in] element The element to release.
        void ReleaseElement(Rml::Element *element) override;
};

class RmlUUIDDrag;
class RmlUUIDDrop;

class RmlUUIDDrag : public Rml::Element {
        friend RmlUUIDDrop;

    private:
        KString uuid_str;

    protected:
        void OnAttributeChange(
            const Rml::ElementAttributes &changed_attributes) override;

    public:
        RmlUUIDDrag(const Rml::String &tag);
        void set_uuid(UUID uuid) { this->uuid_str = uuid.to_string(); }
};

class RmlUUIDDrop : public Rml::Element, public Rml::EventListener {
        friend RmlUUIDDrag;

    private:
        UUID uuid{};

    public:
        RmlUUIDDrop(const Rml::String &tag);
        ~RmlUUIDDrop();
        void ProcessEvent(Rml::Event &event) override;
        UUID get_uuid() { return uuid; }
};

class RmlPreview : public Rml::Element {
    private:
        UUID uuid;
        Rml::Element *image = nullptr;

    protected:
        void OnAttributeChange(
            const Rml::ElementAttributes &changed_attributes) override;

    public:
        RmlPreview(const Rml::String &tag);
        bool set_uuid(UUID uuid);
};

}  // namespace Seed

#endif
