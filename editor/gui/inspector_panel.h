#ifndef _SEED_INSPECTOR_PANEL_H_
#define _SEED_INSPECTOR_PANEL_H_

#include <vector>

#include <RmlUi/Core/DataModelHandle.h>

#include "core/gui/gui.h"
#include "editor/editor/inspector.h"

namespace Seed {

class RmlInspectorField;

class InspectorPanelObserver {
    public:
        virtual void on_inspector_changed() = 0;
        virtual ~InspectorPanelObserver() = default;
};

class InspectorPanel : public RmlGUI {
        friend RmlInspectorField;

    private:
        Ref<InspectorSource> source;
        InspectorPanelObserver *observer = nullptr;
        std::vector<InspectorSection> sections;
        KString title;
        KString status_text;
        KString confirmation_title;
        KString confirmation_message;
        bool available = false;
        bool has_status = false;
        bool show_confirmation = false;
        i32 pending_action_id = -1;
        i32 revision = 0;
        Rml::DataModelHandle view_model;

        void dirty_view_model();
        void close_confirmation();
        void notify_changed();
        InspectorField *get_field(i32 section_index, i32 field_index);
        InspectorAction *get_action(i32 section_index, i32 action_index);

        void rml_save(RML_EVENT_ARGS);
        void rml_commit_field(RML_EVENT_ARGS);
        void rml_invoke_action(RML_EVENT_ARGS);
        void rml_cancel_action(RML_EVENT_ARGS);
        void rml_confirm_action(RML_EVENT_ARGS);

        void register_view_model_types(Rml::DataModelConstructor &constructor);
        void bind_view_model_values(Rml::DataModelConstructor &constructor);
        void bind_view_model_events(Rml::DataModelConstructor &constructor);
        void bind_model(Rml::Context *context) override;

    public:
        void inspect(Ref<InspectorSource> source,
                     InspectorPanelObserver *observer = nullptr);
        void refresh();
};

}  // namespace Seed

#endif
