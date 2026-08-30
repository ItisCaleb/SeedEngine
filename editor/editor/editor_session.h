#ifndef _SEED_EDITOR_SESSION_H_
#define _SEED_EDITOR_SESSION_H_

#include "core/resource/world_setting.h"
#include "core/world/sky.h"
#include "editor/editor/inspector.h"
#include "editor/gui/world_viewport.h"
#include "editor/world/editor_terrain.h"

namespace Seed {

struct EditorViewportInput {
        bool valid = false;
        bool primary_pressed = false;
        bool alt_pressed = false;
};

class EditorSession : public InspectorSource {
    public:
        virtual KStr get_viewport_text() const = 0;
        virtual bool show_viewport_text() const = 0;
        virtual KStr get_viewport_message() const = 0;
        virtual bool show_viewport_empty() const = 0;
        virtual void on_world_changed(bool loaded) = 0;
        virtual bool handle_viewport_pick(const PickResult &result,
                                          const EditorViewportInput &input) = 0;
        virtual void handle_viewport_scroll(f32 delta) = 0;
        ~EditorSession() override = default;
};

class WorldSession : public EditorSession {
    private:
        enum class Property : i32 {
            Name,
            SkyUp,
            SkyDown,
            SkyLeft,
            SkyRight,
            SkyFront,
            SkyBack,
            LightDirection,
            LightDiffuse,
            LightSpecular,
        };

        KString status_text;

        bool get_cubemap_face(Property property, CubemapFace &face) const;
        bool is_texture_asset(UUID uuid) const;
        void apply_directional_light();
        bool update_skybox_face(UUID uuid, CubemapFace face);

    public:
        WorldSession() = default;

        KStr get_name() const override { return "World"; }
        KStr get_status() const override { return status_text; }
        bool is_available() const override;
        void save() override;
        void build_inspector(InspectorBuilder &builder) override;
        bool commit_field(const InspectorField &field) override;
        KStr get_viewport_text() const override { return "-"; }
        bool show_viewport_text() const override { return false; }
        KStr get_viewport_message() const override;
        bool show_viewport_empty() const override;
        void on_world_changed(bool loaded) override;
        bool handle_viewport_pick(const PickResult &result,
                                  const EditorViewportInput &input) override;
        void handle_viewport_scroll(f32 delta) override;
};

class TerrainSession : public EditorSession {
    private:
        enum class Property : i32 {
            DirtyMaps,
            Tool,
            Radius,
            Strength,
            FlattenHeight,
            Palette,
            Diffuse,
            Normal,
        };

        enum class Action : i32 { AddTile, ClearTiles };

        Ref<EditorTerrain> terrain;
        TerrainBrush brush_type = TerrainBrush::Raise;
        TerrainBrushSetting brush_setting;
        KString status_text;
        KString viewport_text = "-";

        bool is_texture_asset(UUID uuid) const;
        bool chunk_exists_at(i32 chunk_x, i32 chunk_y) const;
        bool add_chunk_at(i32 chunk_x, i32 chunk_y);
        bool add_chunk(i32 &chunk_x, i32 &chunk_y);
        void clear_chunks();
        void update_terrain_focus(i32 world_x, i32 world_y);
        bool update_terrain_texture(Property property, UUID uuid);

    public:
        TerrainSession();

        KStr get_name() const override { return "Terrain"; }
        KStr get_status() const override { return status_text; }
        bool is_available() const override;
        void save() override;
        void build_inspector(InspectorBuilder &builder) override;
        bool commit_field(const InspectorField &field) override;
        void invoke_action(i32 action_id) override;
        KStr get_viewport_text() const override { return viewport_text; }
        bool show_viewport_text() const override { return true; }
        KStr get_viewport_message() const override;
        bool show_viewport_empty() const override;
        void on_world_changed(bool loaded) override;
        bool handle_viewport_pick(const PickResult &result,
                                  const EditorViewportInput &input) override;
        void handle_viewport_scroll(f32 delta) override;
};

}  // namespace Seed

#endif
