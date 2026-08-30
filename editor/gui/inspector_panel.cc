#include "inspector_panel.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

namespace Seed {

void InspectorPanel::dirty_view_model() {
    if (view_model) view_model.DirtyAllVariables();
}

void InspectorPanel::close_confirmation() {
    show_confirmation = false;
    pending_action_id = -1;
    confirmation_title.clear();
    confirmation_message.clear();
}

void InspectorPanel::notify_changed() {
    if (observer != nullptr) observer->on_inspector_changed();
}

InspectorField *InspectorPanel::get_field(i32 section_index, i32 field_index) {
    if (section_index < 0 || field_index < 0 ||
        section_index >= (i32)sections.size()) {
        return nullptr;
    }

    std::vector<InspectorField> &fields = sections[(u32)section_index].fields;
    if (field_index >= (i32)fields.size()) return nullptr;
    return &fields[(u32)field_index];
}

InspectorAction *InspectorPanel::get_action(i32 section_index,
                                            i32 action_index) {
    if (section_index < 0 || action_index < 0 ||
        section_index >= (i32)sections.size()) {
        return nullptr;
    }

    std::vector<InspectorAction> &actions =
        sections[(u32)section_index].actions;
    if (action_index >= (i32)actions.size()) return nullptr;
    return &actions[(u32)action_index];
}

void InspectorPanel::rml_save(RML_EVENT_ARGS) {
    if (source.is_null()) return;
    source->save();
    notify_changed();
    refresh();
}

void InspectorPanel::rml_commit_field(RML_EVENT_ARGS) {
    if (source.is_null() || args.size() < 2) return;

    InspectorField *field =
        get_field(args[0].Get<i32>(-1), args[1].Get<i32>(-1));
    if (field == nullptr) return;

    InspectorValue value;
    switch (field->type) {
        case InspectorFieldType::Text:
            value = KString(e.GetParameter<Rml::String>("text", ""));
            break;
        case InspectorFieldType::Integer:
        case InspectorFieldType::Range:
            value = e.GetParameter<i32>("integer", 0);
            break;
        case InspectorFieldType::Float:
            value = e.GetParameter<f32>("floating", 0.0f);
            break;
        case InspectorFieldType::Boolean:
            value = e.GetParameter<bool>("boolean", false);
            break;
        case InspectorFieldType::Vec3:
            value = Vec3{e.GetParameter<f32>("x", 0.0f),
                         e.GetParameter<f32>("y", 0.0f),
                         e.GetParameter<f32>("z", 0.0f)};
            break;
        case InspectorFieldType::Resource:
            value = UUID::from_string(
                e.GetParameter<Rml::String>("resource", ""));
            break;
        case InspectorFieldType::Options: {
            const i32 selected = e.GetParameter<i32>("option", -1);
            bool valid = false;
            for (const InspectorOption &option : field->options()) {
                if (option.value != selected) continue;
                valid = true;
                break;
            }
            if (!valid) return;
            value = selected;
            break;
        }
        case InspectorFieldType::ReadOnly:
            return;
    }

    const InspectorValue previous_value = field->get_value();
    field->set_value(value);
    if (!source->commit_field(*field)) {
        field->set_value(previous_value);
    }
    notify_changed();
    refresh();
}

void InspectorPanel::rml_invoke_action(RML_EVENT_ARGS) {
    if (source.is_null() || args.size() < 2) return;

    InspectorAction *action =
        get_action(args[0].Get<i32>(-1), args[1].Get<i32>(-1));
    if (action == nullptr) return;

    if (action->requires_confirmation) {
        pending_action_id = action->id;
        confirmation_title = action->confirmation_title;
        confirmation_message = action->confirmation_message;
        show_confirmation = true;
        dirty_view_model();
        return;
    }

    source->invoke_action(action->id);
    notify_changed();
    refresh();
}

void InspectorPanel::rml_cancel_action(RML_EVENT_ARGS) {
    close_confirmation();
    dirty_view_model();
}

void InspectorPanel::rml_confirm_action(RML_EVENT_ARGS) {
    if (source.is_null() || pending_action_id < 0) return;

    const i32 action_id = pending_action_id;
    close_confirmation();
    source->invoke_action(action_id);
    notify_changed();
    refresh();
}

void InspectorPanel::register_view_model_types(
    Rml::DataModelConstructor &constructor) {
    if (auto object = constructor.RegisterStruct<InspectorAction>()) {
        object.RegisterMember("label", &InspectorAction::label);
        object.RegisterMember("danger", &InspectorAction::danger);
    }
    constructor.RegisterArray<std::vector<InspectorAction>>();

    constructor.RegisterStruct<InspectorField>();
    constructor.RegisterArray<std::vector<InspectorField>>();

    if (auto object = constructor.RegisterStruct<InspectorSection>()) {
        object.RegisterMember("title", &InspectorSection::title);
        object.RegisterMember("fields", &InspectorSection::fields);
        object.RegisterMember("actions", &InspectorSection::actions);
    }
    constructor.RegisterArray<std::vector<InspectorSection>>();
}

void InspectorPanel::bind_view_model_values(
    Rml::DataModelConstructor &constructor) {
    constructor.Bind("inspector_sections", &sections);
    constructor.Bind("inspector_title", &title);
    constructor.Bind("inspector_available", &available);
    constructor.Bind("status", &status_text);
    constructor.Bind("has_status", &has_status);
    constructor.Bind("show_inspector_confirmation", &show_confirmation);
    constructor.Bind("confirmation_title", &confirmation_title);
    constructor.Bind("confirmation_message", &confirmation_message);
    constructor.Bind("inspector_revision", &revision);
}

void InspectorPanel::bind_view_model_events(
    Rml::DataModelConstructor &constructor) {
    constructor.BindEventCallback("save_inspector", &InspectorPanel::rml_save,
                                  this);
    constructor.BindEventCallback("commit_inspector_field",
                                  &InspectorPanel::rml_commit_field, this);
    constructor.BindEventCallback("invoke_inspector_action",
                                  &InspectorPanel::rml_invoke_action, this);
    constructor.BindEventCallback("cancel_inspector_action",
                                  &InspectorPanel::rml_cancel_action, this);
    constructor.BindEventCallback("confirm_inspector_action",
                                  &InspectorPanel::rml_confirm_action, this);
}

void InspectorPanel::bind_model(Rml::Context *context) {
    Rml::DataModelConstructor constructor =
        context->CreateDataModel("inspector");
    if (!constructor) return;

    register_view_model_types(constructor);
    bind_view_model_values(constructor);
    bind_view_model_events(constructor);
    view_model = constructor.GetModelHandle();
    refresh();
}

void InspectorPanel::inspect(Ref<InspectorSource> source,
                             InspectorPanelObserver *observer) {
    this->source = source;
    this->observer = observer;
    close_confirmation();
    refresh();
}

void InspectorPanel::refresh() {
    sections.clear();
    title.clear();
    status_text.clear();
    available = false;

    if (source.is_valid()) {
        title = source->get_name().string();
        status_text = source->get_status().string();
        available = source->is_available();
        if (available) {
            InspectorBuilder builder(sections);
            source->build_inspector(builder);
        }
    }
    has_status = !status_text.is_empty();
    revision++;
    dirty_view_model();
}

}  // namespace Seed
