#ifndef _SEED_WORLD_EDITOR_H_
#define _SEED_WORLD_EDITOR_H_

#include <string>
#include <vector>
#include <RmlUi/Core/DataModelHandle.h>
#include "core/container/kstring.h"
#include "core/gui/gui.h"
#include "core/math/vec3.h"
#include "core/resource/world_setting.h"
#include "editor_terrain.h"

namespace Seed {

class EditorWorld;
class WorldRenderer;

enum class WorldEditorMode { World, Terrain };

class WorldEditor : public RmlGUI {
        friend WorldRenderer;

    private:
        struct SceneObjectView {
                KString name;
                KString asset;
                bool selected = false;
                i32 chunk_index = -1;
                i32 object_index = -1;
        };

        struct TerrainPaletteView {
                i32 slot = 0;
                bool selected = false;
        };

        EditorWorld *current_world = nullptr;
        WorldRenderer *renderer = nullptr;
        WorldEditorMode active_mode = WorldEditorMode::World;
        TerrainBrush brush_type = TerrainBrush::Raise;
        TerrainBrushSetting brush_setting;
        i32 last_pick_x = 0;
        i32 last_pick_y = 0;
        bool last_pick_valid = false;
        i32 selected_static_chunk = -1;
        i32 selected_static_object = -1;
        std::string status_text;
        KString dirty_maps_text;
        KString viewport_message;
        KString world_text;
        KString selected_name;
        i32 selected_x = 0;
        i32 selected_y = 0;
        i32 selected_z = 0;
        KString selected_asset;
        std::vector<SceneObjectView> scene_objects;
        std::vector<TerrainPaletteView> terrain_palette;
        UUID selected_terrain_diffuse;
        UUID selected_terrain_normal;
        bool has_world = false;
        bool has_status = false;
        bool has_selected_object = false;
        bool show_scene_empty = true;
        bool show_viewport_empty = true;
        bool show_clear_tiles = false;
        bool view_model_types_registered = false;
        f32 viewport_scroll_delta = 0.0f;
        Rml::DataModelHandle view_model;

        void rebind_view_model();
        void sync_view_model();
        void dirty_view_model();
        void set_status(std::string status);
        void sync_dirty_maps();
        void reset_selection();
        void rml_set_mode(RML_EVENT_ARGS);
        void rml_set_tool(RML_EVENT_ARGS);
        void rml_save_world(RML_EVENT_ARGS);
        void rml_add_tile(RML_EVENT_ARGS);
        void rml_request_clear_tiles(RML_EVENT_ARGS);
        void rml_cancel_clear_tiles(RML_EVENT_ARGS);
        void rml_confirm_clear_tiles(RML_EVENT_ARGS);
        void rml_commit_world_settings(RML_EVENT_ARGS);
        void rml_set_skybox_face(RML_EVENT_ARGS);
        void rml_select_terrain_layer(RML_EVENT_ARGS);
        void rml_set_terrain_texture(RML_EVENT_ARGS);
        void rml_select_scene_object(RML_EVENT_ARGS);
        void rml_commit_selected_object(RML_EVENT_ARGS);
        void rml_viewport_pick(RML_EVENT_ARGS);
        void rml_viewport_scroll(RML_EVENT_ARGS);

        void bind_model(Rml::Context *context) override;
        void register_view_model_types(Rml::DataModelConstructor &constructor);
        void bind_view_model_values(Rml::DataModelConstructor &constructor);
        void bind_view_model_events(Rml::DataModelConstructor &constructor);

        void sync_scene_view_model();
        void sync_selected_object_view_model();
        void sync_terrain_palette_view_model();
        bool viewport_event_to_pixel(Rml::Event &event, i32 &image_x,
                                     i32 &image_y) const;
        bool pick_world_at_pixel(i32 image_x, i32 image_y, i32 &world_x,
                                 i32 &world_y) const;
        bool update_pick_from_event(Rml::Event &event);
        bool chunk_exists_at(i32 chunk_x, i32 chunk_y) const;
        bool add_chunk_at(i32 chunk_x, i32 chunk_y);
        KString static_model_label(UUID uuid) const;
        void select_static_object(u32 chunk_index, u32 object_index);
        void add_chunk();
        void clear_tiles();
        bool save_current_world();

    public:
        void init();
        ~WorldEditor();
        bool load_world(const UUID uuid);
        bool is_viewport_hovered() const;
        bool get_camera_focus(Vec3 &target) const;
        f32 consume_viewport_scroll();
        EditorWorld *get_current_world() { return current_world; }
};

}  // namespace Seed

#endif
