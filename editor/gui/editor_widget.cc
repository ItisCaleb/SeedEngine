#include "editor_widget.h"

#include <RmlUi/Core/Factory.h>
#include "core/gui/gui_engine.h"
#include "core/resource/resource_loader.h"

namespace Seed {

void EditorRmlElementInstancer::RegisterElements() {
    Rml::Factory::RegisterElementInstancer(RML_UUID_DRAG, this);
    Rml::Factory::RegisterElementInstancer(RML_UUID_DROP, this);
    Rml::Factory::RegisterElementInstancer(RML_PREVIEW, this);
}

Rml::ElementPtr EditorRmlElementInstancer::InstanceElement(
    Rml::Element *parent, const Rml::String &tag,
    const Rml::XMLAttributes &attributes) {
    if (tag == RML_UUID_DRAG) {
        return Rml::ElementPtr(new RmlUUIDDrag(tag));
    } else if (tag == RML_UUID_DROP) {
        return Rml::ElementPtr(new RmlUUIDDrop(tag));
    } else if (tag == RML_PREVIEW) {
        return Rml::ElementPtr(new RmlPreview(tag));
    }
    return nullptr;
}

void EditorRmlElementInstancer::ReleaseElement(Rml::Element *element) {
    delete element;
}

RmlUUIDDrag::RmlUUIDDrag(const Rml::String &tag) : Rml::Element(tag) {
    SetProperty("drag", "clone");
}

void RmlUUIDDrag::OnAttributeChange(
    const Rml::ElementAttributes &changed_attributes) {
    Rml::Element::OnAttributeChange(changed_attributes);

    auto it = changed_attributes.find("value");
    if (it == changed_attributes.end()) {
        return;
    }

    const Rml::String value = it->second.Get<Rml::String>();
    set_uuid(UUID::from_string(value));
}

RmlUUIDDrop::RmlUUIDDrop(const Rml::String &tag) : Rml::Element(tag) {
    AddEventListener(Rml::EventId::Dragdrop, this);
}

RmlUUIDDrop::~RmlUUIDDrop() {
    RemoveEventListener(Rml::EventId::Dragdrop, this);
}

void RmlUUIDDrop::ProcessEvent(Rml::Event &event) {
    if (event.GetId() != Rml::EventId::Dragdrop) {
        return;
    }
    Rml::Element *drag_element = static_cast<Rml::Element *>(
        event.GetParameter<void *>("drag_element", NULL));
    RmlUUIDDrag *drag = dynamic_cast<RmlUUIDDrag *>(drag_element);
    if (!drag) {
        return;
    }

    SetAttribute("value",
                 Rml::String(drag->uuid_str.data(), drag->uuid_str.size()));
    Rml::Dictionary params;
    params["value"] = Rml::String(drag->uuid_str.data(), drag->uuid_str.size());
    DispatchEvent("change", params);
}

RmlPreview::RmlPreview(const Rml::String &tag) : Rml::Element(tag) {
    SetInnerRML("<img class=\"preview-image\" />");
    image = GetFirstChild();
}
void RmlPreview::OnAttributeChange(
    const Rml::ElementAttributes &changed_attributes) {
    Rml::Element::OnAttributeChange(changed_attributes);

    auto it = changed_attributes.find("value");
    if (it == changed_attributes.end()) {
        return;
    }

    const Rml::String value = it->second.Get<Rml::String>();
    set_uuid(UUID::from_string(value));
}

bool RmlPreview::set_uuid(UUID uuid) {
    this->uuid = uuid;
    if (image == nullptr) return false;

    image->SetAttribute("src", "internal://preview-default");

    if (uuid.is_null()) return false;
    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    ResourceEntry *entry = entries.get_entry(uuid);
    if (entry == nullptr || entry->type_id != type_id<Texture>()) {
        return false;
    }

    KString preview_name = fmt::format("preview-{}", uuid.to_string());
    Ref<Texture> preview = GuiEngine::get_instance()->get_texture(preview_name);
    if (preview.is_null()) {
        preview.create(TextureType::TEXTURE_2D, 48, 48, PixelFormat::RGBA);
        GuiEngine::get_instance()->add_texture(preview_name, preview);
        /* use CPU downscale */
        /* we probably don't want to upload to GPU just for this */
        ThreadPool::get_instance()->add_work([=](void *) {
            Ref<Image> preview_img =
                Image::load_from_file(entry->real_path(), true);
            preview_img->downscale(48, 48)->upload(preview);
        });
    }

    image->SetAttribute("src", fmt::format("internal://{}", preview_name));
    return true;
}

}  // namespace Seed
