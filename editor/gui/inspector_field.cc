#include "inspector_field.h"

#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <fmt/format.h>

#include "editor/editor.h"

namespace Seed {

RmlInspectorField::RmlInspectorField(const Rml::String &tag)
    : Rml::Element(tag) {
    SetProperty("display", "block");
    AddEventListener(Rml::EventId::Change, this);
    AddEventListener(Rml::EventId::Click, this);
}

RmlInspectorField::~RmlInspectorField() {
    RemoveEventListener(Rml::EventId::Change, this);
    RemoveEventListener(Rml::EventId::Click, this);
}

Rml::String RmlInspectorField::encode(KStr value) const {
    return Rml::StringUtilities::EncodeRml(
        Rml::String(value.data(), value.length()));
}

void RmlInspectorField::rebuild() {
    InspectorField *field = System::gEditor->get_inspector_panel().get_field(
        section_index, field_index);
    if (field == nullptr) {
        SetInnerRML("");
        return;
    }

    const Rml::String label = encode(field->label);
    Rml::String content;
    SetClass("resource", field->type == InspectorFieldType::Resource);

    switch (field->type) {
        case InspectorFieldType::Text:
            content = fmt::format(
                "<label>{}</label><input class=\"inspector-input\" "
                "type=\"text\" value=\"{}\" />",
                label, encode(field->text()));
            break;
        case InspectorFieldType::Integer:
            content = fmt::format(
                "<label>{}</label><input class=\"inspector-input\" "
                "type=\"text\" value=\"{}\" />",
                label, field->integer());
            break;
        case InspectorFieldType::Float:
            content = fmt::format(
                "<label>{}</label><input class=\"inspector-input\" "
                "type=\"text\" value=\"{}\" />",
                label, field->floating());
            break;
        case InspectorFieldType::Boolean:
            content = fmt::format(
                "<label class=\"boolean-field\"><input type=\"checkbox\" "
                "{} /><span>{}</span></label>",
                field->boolean() ? "checked" : "", label);
            break;
        case InspectorFieldType::Vec3: {
            const Vec3 &vector = field->vector();
            const bool rgb =
                field->vector_components() == InspectorVectorComponents::RGB;
            content = fmt::format(
                "<div class=\"vector-field\"><div "
                "class=\"vector-label\">{}</div>"
                "<div class=\"axis-row\"><span>{}</span><input "
                "type=\"text\" value=\"{}\" /></div>"
                "<div class=\"axis-row\"><span>{}</span><input "
                "type=\"text\" value=\"{}\" /></div>"
                "<div class=\"axis-row\"><span>{}</span><input "
                "type=\"text\" value=\"{}\" /></div></div>",
                label, rgb ? "R" : "X", vector.x, rgb ? "G" : "Y", vector.y,
                rgb ? "B" : "Z", vector.z);
            break;
        }
        case InspectorFieldType::Resource: {
            const Rml::String resource = field->resource().to_string();
            content = fmt::format(
                "<div class=\"inspector-resource\"><div "
                "class=\"inspector-resource-label\">{}</div>"
                "<x-uuid-drop class=\"inspector-resource-drop\" "
                "value=\"{}\"><x-preview class=\"inspector-resource-preview\" "
                "value=\"{}\"></x-preview></x-uuid-drop></div>",
                label, resource, resource);
            break;
        }
        case InspectorFieldType::Range:
            content = fmt::format(
                "<div class=\"range-field\"><label>{} {}</label>"
                "<input class=\"range\" type=\"range\" min=\"{}\" "
                "max=\"{}\" step=\"{}\" value=\"{}\" /></div>",
                label, field->integer(), field->range_min(), field->range_max(),
                field->range_step(), field->integer());
            break;
        case InspectorFieldType::ReadOnly:
            content = fmt::format(
                "<div class=\"metric-row\"><span>{}</span>"
                "<span class=\"metric-value\">{}</span></div>",
                label, encode(field->text()));
            break;
        case InspectorFieldType::Options:
            content = fmt::format(
                "<div class=\"options-field\"><div "
                "class=\"field-title\">{}</div>"
                "<div class=\"inspector-option-grid\">",
                label);
            for (const InspectorOption &option : field->options()) {
                content += fmt::format(
                    "<button class=\"inspector-option{}\" option=\"{}\">"
                    "{}</button>",
                    option.value == field->integer() ? " active" : "",
                    option.value,
                    encode(option.label));
            }
            content += "</div></div>";
            break;
    }

    SetInnerRML(content);
}

void RmlInspectorField::OnAttributeChange(
    const Rml::ElementAttributes &changed_attributes) {
    Rml::Element::OnAttributeChange(changed_attributes);
    if (changed_attributes.count("section") == 0 &&
        changed_attributes.count("field") == 0 &&
        changed_attributes.count("revision") == 0) {
        return;
    }

    section_index = GetAttribute<i32>("section", -1);
    field_index = GetAttribute<i32>("field", -1);
    rebuild();
}

void RmlInspectorField::dispatch_change(Rml::Event &event) {
    InspectorField *field = System::gEditor->get_inspector_panel().get_field(
        section_index, field_index);
    if (field == nullptr) return;

    Rml::Dictionary parameters;
    if (field->type == InspectorFieldType::Resource) {
        parameters["resource"] = event.GetParameter<Rml::String>("value", "");
    } else {
        Rml::ElementList inputs;
        GetElementsByTagName(inputs, "input");
        if (inputs.empty()) return;

        auto input = [&inputs](u32 index) {
            return static_cast<Rml::ElementFormControlInput *>(inputs[index]);
        };
        switch (field->type) {
            case InspectorFieldType::Text:
                parameters["text"] = input(0)->GetValue();
                break;
            case InspectorFieldType::Integer:
            case InspectorFieldType::Range:
                parameters["integer"] =
                    Rml::Variant(input(0)->GetValue()).Get<i32>();
                break;
            case InspectorFieldType::Float:
                parameters["floating"] =
                    Rml::Variant(input(0)->GetValue()).Get<f32>();
                break;
            case InspectorFieldType::Boolean:
                parameters["boolean"] = input(0)->IsSubmitted();
                break;
            case InspectorFieldType::Vec3:
                if (inputs.size() < 3) return;
                parameters["x"] = Rml::Variant(input(0)->GetValue()).Get<f32>();
                parameters["y"] = Rml::Variant(input(1)->GetValue()).Get<f32>();
                parameters["z"] = Rml::Variant(input(2)->GetValue()).Get<f32>();
                break;
            default:
                return;
        }
    }

    event.StopImmediatePropagation();
    DispatchEvent(Rml::EventId::Change, parameters);
}

void RmlInspectorField::ProcessEvent(Rml::Event &event) {
    if (event.GetId() == Rml::EventId::Change) {
        if (event.GetTargetElement() == this) return;
        dispatch_change(event);
        return;
    }
    if (event.GetId() != Rml::EventId::Click) return;

    Rml::Element *target = event.GetTargetElement();
    while (target != nullptr && target != this &&
           !target->HasAttribute("option")) {
        target = target->GetParentNode();
    }
    if (target == nullptr || target == this) return;

    Rml::Dictionary parameters;
    parameters["option"] = target->GetAttribute<i32>("option", -1);
    event.StopImmediatePropagation();
    DispatchEvent(Rml::EventId::Change, parameters);
}

}  // namespace Seed
