#include "inspector.h"

#include <algorithm>

namespace Seed {

KStr InspectorField::text() const {
    if (type == InspectorFieldType::ReadOnly) {
        return std::get<KString>(storage);
    }
    return *std::get<KString *>(storage);
}

i32 InspectorField::integer() const {
    if (type == InspectorFieldType::Range) {
        return std::get<RangeValue>(storage).value.get();
    }
    if (type == InspectorFieldType::Options) {
        return std::get<OptionsValue>(storage).value.get();
    }
    return std::get<IntegerBinding>(storage).get();
}

f32 InspectorField::floating() const { return *std::get<f32 *>(storage); }

bool InspectorField::boolean() const { return *std::get<bool *>(storage); }

const Vec3 &InspectorField::vector() const {
    return *std::get<VectorValue>(storage).value;
}

UUID InspectorField::resource() const { return *std::get<UUID *>(storage); }

i32 InspectorField::range_min() const {
    return std::get<RangeValue>(storage).min;
}

i32 InspectorField::range_max() const {
    return std::get<RangeValue>(storage).max;
}

i32 InspectorField::range_step() const {
    return std::get<RangeValue>(storage).step;
}

InspectorVectorComponents InspectorField::vector_components() const {
    return std::get<VectorValue>(storage).components;
}

const std::vector<InspectorOption> &InspectorField::options() const {
    return std::get<OptionsValue>(storage).options;
}

InspectorValue InspectorField::get_value() const {
    switch (type) {
        case InspectorFieldType::Text:
        case InspectorFieldType::ReadOnly:
            return text().string();
        case InspectorFieldType::Integer:
        case InspectorFieldType::Range:
        case InspectorFieldType::Options:
            return integer();
        case InspectorFieldType::Float:
            return floating();
        case InspectorFieldType::Boolean:
            return boolean();
        case InspectorFieldType::Vec3:
            return vector();
        case InspectorFieldType::Resource:
            return resource();
    }
    return KString();
}

void InspectorField::set_value(const InspectorValue &value) {
    switch (type) {
        case InspectorFieldType::Text:
            *std::get<KString *>(storage) = std::get<KString>(value);
            break;
        case InspectorFieldType::Integer:
            std::get<IntegerBinding>(storage).set(std::get<i32>(value));
            break;
        case InspectorFieldType::Float:
            *std::get<f32 *>(storage) = std::get<f32>(value);
            break;
        case InspectorFieldType::Boolean:
            *std::get<bool *>(storage) = std::get<bool>(value);
            break;
        case InspectorFieldType::Vec3:
            *std::get<VectorValue>(storage).value = std::get<Vec3>(value);
            break;
        case InspectorFieldType::Resource:
            *std::get<UUID *>(storage) = std::get<UUID>(value);
            break;
        case InspectorFieldType::Range: {
            RangeValue &range = std::get<RangeValue>(storage);
            range.value.set(std::clamp(std::get<i32>(value), range.min,
                                       range.max));
            break;
        }
        case InspectorFieldType::Options:
            std::get<OptionsValue>(storage).value.set(std::get<i32>(value));
            break;
        case InspectorFieldType::ReadOnly:
            break;
    }
}

void InspectorBuilder::begin_section(KStr title) {
    InspectorSection &section = sections.emplace_back();
    section.title = title.string();
    current_section = &section;
}

InspectorField &InspectorBuilder::add_field(i32 id, KStr label,
                                            InspectorFieldType type) {
    if (current_section == nullptr) begin_section("");

    InspectorField &field = current_section->fields.emplace_back();
    field.id = id;
    field.label = label.string();
    field.type = type;
    return field;
}

InspectorField &InspectorBuilder::text(i32 id, KStr label, KString &value) {
    InspectorField &field = add_field(id, label, InspectorFieldType::Text);
    field.storage = &value;
    return field;
}

InspectorField &InspectorBuilder::integer(i32 id, KStr label, i32 &value) {
    InspectorField &field = add_field(id, label, InspectorFieldType::Integer);
    field.storage = InspectorField::IntegerBinding::bind(value);
    return field;
}

InspectorField &InspectorBuilder::floating(i32 id, KStr label, f32 &value) {
    InspectorField &field = add_field(id, label, InspectorFieldType::Float);
    field.storage = &value;
    return field;
}

InspectorField &InspectorBuilder::boolean(i32 id, KStr label, bool &value) {
    InspectorField &field = add_field(id, label, InspectorFieldType::Boolean);
    field.storage = &value;
    return field;
}

InspectorField &InspectorBuilder::vec3(i32 id, KStr label, Vec3 &value,
                                       InspectorVectorComponents components) {
    InspectorField &field = add_field(id, label, InspectorFieldType::Vec3);
    field.storage = InspectorField::VectorValue{&value, components};
    return field;
}

InspectorField &InspectorBuilder::resource(i32 id, KStr label, UUID &value) {
    InspectorField &field = add_field(id, label, InspectorFieldType::Resource);
    field.storage = &value;
    return field;
}

InspectorField &InspectorBuilder::read_only(i32 id, KStr label, KStr value) {
    InspectorField &field = add_field(id, label, InspectorFieldType::ReadOnly);
    field.storage = value.string();
    return field;
}

void InspectorBuilder::option(InspectorField &field, KStr label, i32 value) {
    InspectorOption &option =
        std::get<InspectorField::OptionsValue>(field.storage)
            .options.emplace_back();
    option.label = label.string();
    option.value = value;
}

void InspectorBuilder::action(i32 id, KStr label, bool danger) {
    if (current_section == nullptr) begin_section("");

    InspectorAction &action = current_section->actions.emplace_back();
    action.id = id;
    action.label = label.string();
    action.danger = danger;
}

void InspectorBuilder::confirmation_action(i32 id, KStr label, KStr title,
                                           KStr message, bool danger) {
    action(id, label, danger);
    InspectorAction &action = current_section->actions.back();
    action.requires_confirmation = true;
    action.confirmation_title = title.string();
    action.confirmation_message = message.string();
}

KStr InspectorSource::get_status() const { return ""; }

bool InspectorSource::is_available() const { return true; }

bool InspectorSource::commit_field(const InspectorField &) { return true; }

void InspectorSource::invoke_action(i32) {}

void InspectorSource::save() {}

}  // namespace Seed
