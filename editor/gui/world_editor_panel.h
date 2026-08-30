#ifndef _SEED_WORLD_EDITOR_PANEL_H_
#define _SEED_WORLD_EDITOR_PANEL_H_

#include <vector>

#include <RmlUi/Core/DataModelHandle.h>

#include "core/gui/gui.h"
#include "editor/editor/editor_session.h"
#include "editor/gui/inspector_panel.h"

namespace Seed {

class WorldEditorPanel : public RmlGUI, public InspectorPanelObserver {
    private:
        struct SessionView {
                KString name;
                bool active = false;
        };

        InspectorPanel &inspector_panel;
        std::vector<Ref<EditorSession>> sessions;
        Ref<EditorSession> active_session;
        std::vector<SessionView> session_views;
        KString viewport_text = "-";
        KString viewport_message;
        bool show_viewport_text = false;
        bool show_viewport_empty = true;
        Rml::DataModelHandle view_model;

        void activate_session(u32 index);
        void refresh_session_views();
        void sync_view_state();
        void dirty_view_model();
        void notify_world_changed(bool loaded);

        void rml_set_session(RML_EVENT_ARGS);
        void rml_viewport_pick(RML_EVENT_ARGS);
        void rml_viewport_scroll(RML_EVENT_ARGS);

        void register_view_model_types(Rml::DataModelConstructor &constructor);
        void bind_view_model_values(Rml::DataModelConstructor &constructor);
        void bind_view_model_events(Rml::DataModelConstructor &constructor);
        void bind_model(Rml::Context *context) override;
        void on_inspector_changed() override;

    public:
        explicit WorldEditorPanel(InspectorPanel &inspector_panel);

        void init();
        void add_session(Ref<EditorSession> session);
        void on_project_changed();
        void on_world_changed(bool loaded);
};

}  // namespace Seed

#endif
