#include "world_editor_panel.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

#include "core/input.h"
#include "core/system.h"
#include "editor/editor.h"

namespace Seed {

void WorldEditorPanel::activate_session(u32 index) {
    if (index >= sessions.size()) return;

    active_session = sessions[index];
    inspector_panel.inspect(ref_cast<InspectorSource>(active_session), this);
    refresh_session_views();
    sync_view_state();
    dirty_view_model();
}

void WorldEditorPanel::refresh_session_views() {
    session_views.clear();
    session_views.reserve(sessions.size());
    for (Ref<EditorSession> &session : sessions) {
        SessionView &view = session_views.emplace_back();
        view.name = session->get_name().string();
        view.active =
            active_session.is_valid() && active_session.ptr() == session.ptr();
    }
}

void WorldEditorPanel::sync_view_state() {
    viewport_text = "-";
    viewport_message.clear();
    show_viewport_text = false;
    show_viewport_empty = true;
    if (active_session.is_null()) return;

    viewport_text = active_session->get_viewport_text().string();
    viewport_message = active_session->get_viewport_message().string();
    show_viewport_text = active_session->show_viewport_text();
    show_viewport_empty = active_session->show_viewport_empty();
}

void WorldEditorPanel::dirty_view_model() {
    if (view_model) view_model.DirtyAllVariables();
}

void WorldEditorPanel::notify_world_changed(bool loaded) {
    for (Ref<EditorSession> &session : sessions) {
        session->on_world_changed(loaded);
    }
    inspector_panel.refresh();
    sync_view_state();
    dirty_view_model();
}

void WorldEditorPanel::rml_set_session(RML_EVENT_ARGS) {
    if (args.empty()) return;
    activate_session((u32)args[0].Get<i32>(-1));
}

void WorldEditorPanel::rml_viewport_pick(RML_EVENT_ARGS) {
    if (active_session.is_null()) return;

    WorldViewport &viewport = System::gEditor->get_world_viewport();
    i32 image_x = 0;
    i32 image_y = 0;
    PickResult result{};
    EditorViewportInput input;
    input.valid = viewport.event_to_pixel(e, image_x, image_y) &&
                  viewport.pick_at_pixel(image_x, image_y, result);
    input.primary_pressed = System::gInput != nullptr &&
                            System::gInput->is_mouse_pressed(MouseEvent::LEFT);
    input.alt_pressed = e.GetParameter<bool>("alt_key", false);

    if (active_session->handle_viewport_pick(result, input)) {
        inspector_panel.refresh();
    }
    sync_view_state();
    dirty_view_model();
}

void WorldEditorPanel::rml_viewport_scroll(RML_EVENT_ARGS) {
    if (active_session.is_valid()) {
        active_session->handle_viewport_scroll(
            e.GetParameter<f32>("wheel_delta_y", 0.0f));
    }
    e.StopPropagation();
}

void WorldEditorPanel::register_view_model_types(
    Rml::DataModelConstructor &constructor) {
    if (auto object = constructor.RegisterStruct<SessionView>()) {
        object.RegisterMember("name", &SessionView::name);
        object.RegisterMember("active", &SessionView::active);
    }
    constructor.RegisterArray<std::vector<SessionView>>();
}

void WorldEditorPanel::bind_view_model_values(
    Rml::DataModelConstructor &constructor) {
    constructor.Bind("editor_sessions", &session_views);
    constructor.Bind("viewport_text", &viewport_text);
    constructor.Bind("viewport_message", &viewport_message);
    constructor.Bind("show_viewport_text", &show_viewport_text);
    constructor.Bind("show_viewport_empty", &show_viewport_empty);
}

void WorldEditorPanel::bind_view_model_events(
    Rml::DataModelConstructor &constructor) {
    constructor.BindEventCallback("set_session",
                                  &WorldEditorPanel::rml_set_session, this);
    constructor.BindEventCallback("viewport_pick",
                                  &WorldEditorPanel::rml_viewport_pick, this);
    constructor.BindEventCallback("viewport_scroll",
                                  &WorldEditorPanel::rml_viewport_scroll, this);
}

void WorldEditorPanel::bind_model(Rml::Context *context) {
    Rml::DataModelConstructor constructor =
        context->CreateDataModel("world_editor");
    if (!constructor) return;

    register_view_model_types(constructor);
    bind_view_model_values(constructor);
    bind_view_model_events(constructor);
    view_model = constructor.GetModelHandle();
    sync_view_state();
    dirty_view_model();
}

void WorldEditorPanel::on_inspector_changed() {
    sync_view_state();
    dirty_view_model();
}

void WorldEditorPanel::add_session(Ref<EditorSession> session) {
    if (session.is_null()) return;

    sessions.push_back(session);
    if (active_session.is_null()) {
        activate_session((u32)sessions.size() - 1);
    } else {
        refresh_session_views();
        dirty_view_model();
    }
}

void WorldEditorPanel::on_project_changed() { notify_world_changed(false); }

void WorldEditorPanel::on_world_changed(bool loaded) {
    notify_world_changed(loaded);
}

WorldEditorPanel::WorldEditorPanel(InspectorPanel &inspector_panel)
    : inspector_panel(inspector_panel) {}

void WorldEditorPanel::init() {
    Ref<WorldSession> world_session;
    world_session.create();
    add_session(ref_cast<EditorSession>(world_session));

    Ref<TerrainSession> terrain_session;
    terrain_session.create();
    add_session(ref_cast<EditorSession>(terrain_session));
}

}  // namespace Seed
