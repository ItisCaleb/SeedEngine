#ifndef _SEED_INSPECTOR_H_
#define _SEED_INSPECTOR_H_

#include <type_traits>
#include <variant>
#include <vector>

#include "core/container/kstring.h"
#include "core/math/vec3.h"
#include "core/misc/uuid.h"
#include "core/ref.h"
#include "core/types.h"

namespace Seed {

enum class InspectorFieldType : u8 {
    Text,
    Integer,
    Float,
    Boolean,
    Vec3,
    Resource,
    Range,
    ReadOnly,
    Options,
};

enum class InspectorVectorComponents : u8 { XYZ, RGB };

struct InspectorOption {
        KString label;
        i32 value = 0;
};

using InspectorValue = std::variant<KString, i32, f32, bool, Vec3, UUID>;

class InspectorField {
    public:
        i32 id = -1;
        InspectorFieldType type = InspectorFieldType::ReadOnly;
        KString label;

    private:
        struct IntegerBinding {
                void *value = nullptr;
                i32 (*read)(const void *) = nullptr;
                void (*write)(void *, i32) = nullptr;

                template <typename T>
                static IntegerBinding bind(T &value) {
                    static_assert(!std::is_const_v<T>);
                    static_assert((std::is_integral_v<T> &&
                                   !std::is_same_v<T, bool>) ||
                                  std::is_enum_v<T>);
                    return {
                        &value,
                        [](const void *pointer) {
                            return (i32)*static_cast<const T *>(pointer);
                        },
                        [](void *pointer, i32 value) {
                            *static_cast<T *>(pointer) = (T)value;
                        },
                    };
                }

                i32 get() const { return read(value); }
                void set(i32 new_value) const { write(value, new_value); }
        };

        struct VectorValue {
                Vec3 *value = nullptr;
                InspectorVectorComponents components =
                    InspectorVectorComponents::XYZ;
        };

        struct RangeValue {
                IntegerBinding value;
                i32 min = 0;
                i32 max = 100;
                i32 step = 1;
        };

        struct OptionsValue {
                IntegerBinding value;
                std::vector<InspectorOption> options;
        };

        /* Editable values are owned by the source and stay bound until the
         * inspector is refreshed. Read-only text is the only owned value. */
        using Storage =
            std::variant<std::monostate, KString *, IntegerBinding, f32 *,
                         bool *, VectorValue, UUID *, RangeValue, KString,
                         OptionsValue>;

        Storage storage;

        friend class InspectorBuilder;

    public:
        KStr text() const;
        i32 integer() const;
        f32 floating() const;
        bool boolean() const;
        const Vec3 &vector() const;
        UUID resource() const;
        i32 range_min() const;
        i32 range_max() const;
        i32 range_step() const;
        InspectorVectorComponents vector_components() const;
        const std::vector<InspectorOption> &options() const;

        InspectorValue get_value() const;
        void set_value(const InspectorValue &value);
};

struct InspectorAction {
        i32 id = -1;
        KString label;
        bool danger = false;
        bool requires_confirmation = false;
        KString confirmation_title;
        KString confirmation_message;
};

struct InspectorSection {
        KString title;
        std::vector<InspectorField> fields;
        std::vector<InspectorAction> actions;
};

class InspectorBuilder {
    private:
        std::vector<InspectorSection> &sections;
        InspectorSection *current_section = nullptr;

        InspectorField &add_field(i32 id, KStr label, InspectorFieldType type);

    public:
        explicit InspectorBuilder(std::vector<InspectorSection> &sections)
            : sections(sections) {}

        void begin_section(KStr title);
        InspectorField &text(i32 id, KStr label, KString &value);
        InspectorField &integer(i32 id, KStr label, i32 &value);
        InspectorField &floating(i32 id, KStr label, f32 &value);
        InspectorField &boolean(i32 id, KStr label, bool &value);
        InspectorField &vec3(i32 id, KStr label, Vec3 &value,
                             InspectorVectorComponents components =
                                 InspectorVectorComponents::XYZ);
        InspectorField &resource(i32 id, KStr label, UUID &value);

        template <typename T>
        InspectorField &range(i32 id, KStr label, T &value, i32 min_value,
                              i32 max_value, i32 step_value = 1) {
            InspectorField &field =
                add_field(id, label, InspectorFieldType::Range);
            field.storage = InspectorField::RangeValue{
                InspectorField::IntegerBinding::bind(value), min_value,
                max_value, step_value};
            return field;
        }

        InspectorField &read_only(i32 id, KStr label, KStr value);

        template <typename T>
        InspectorField &options(i32 id, KStr label, T &value) {
            InspectorField &field =
                add_field(id, label, InspectorFieldType::Options);
            field.storage = InspectorField::OptionsValue{
                InspectorField::IntegerBinding::bind(value), {}};
            return field;
        }

        void option(InspectorField &field, KStr label, i32 value);
        void action(i32 id, KStr label, bool danger = false);
        void confirmation_action(i32 id, KStr label, KStr title, KStr message,
                                 bool danger = false);
};

class InspectorSource : public RefCounted {
    public:
        virtual KStr get_name() const = 0;
        virtual KStr get_status() const;
        virtual bool is_available() const;
        virtual void build_inspector(InspectorBuilder &builder) = 0;
        virtual bool commit_field(const InspectorField &field);
        virtual void invoke_action(i32 action_id);
        virtual void save();
        virtual ~InspectorSource() = default;
};

}  // namespace Seed

#endif
