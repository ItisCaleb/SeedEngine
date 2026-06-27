#ifndef _SEED_WORLD_EDITOR_H_
#define _SEED_WORLD_EDITOR_H_

#include <set>
#include <string>
#include "core/io/path.h"
#include "core/resource/image.h"
#include "core/resource/resource_entry.h"
#include "core/resource/texture.h"

struct ImVec2;

namespace Seed {

namespace EditorUI {

struct TexturePreview;

}  // namespace EditorUI

class EditorWorld;
class WorldRenderer;

enum class WorldEditorMode { World, Terrain };

enum class WorldTerrainTool { Raise, Lower, Smooth, Flatten, Pick, Splat };

enum class TerrainPaletteMap { Albedo, Normal };

class WorldEditor {
        friend WorldRenderer;

    private:
        EditorWorld *current_world = nullptr;
        Ref<Texture> screen_texture;
        Ref<Texture> screen_depth;
        Ref<MappableTexture> picking_texture;
        WorldRenderer *renderer = nullptr;
        ResourceEntry *current_entry = nullptr;
        Path current_world_path;
        u32 screen_width = 1024;
        u32 screen_height = 768;
        bool preview_terrain_dirty = false;
        WorldEditorMode active_mode = WorldEditorMode::World;
        WorldTerrainTool terrain_tool = WorldTerrainTool::Raise;
        f32 brush_radius = 12.0f;
        f32 brush_strength = 0.25f;
        f32 flatten_height = 128.0f;
        i32 selected_terrain_palette_slot = -1;
        i32 last_pick_x = 0;
        i32 last_pick_y = 0;
        bool last_pick_valid = false;
        i32 last_image_x = 0;
        i32 last_image_y = 0;
        bool last_image_valid = false;
        bool heightmaps_dirty = false;
        bool controlmaps_dirty = false;
        i32 selected_static_chunk = -1;
        i32 selected_static_object = -1;
        std::set<u32> dirty_heightmaps;
        std::set<u32> dirty_controlmaps;
        std::string status_text;

        ResourceEntry *find_entry_for_path(const Path &path);
        void set_current_world_inspector();
        void mark_preview_terrain_dirty();
        void save_dirty_heightmaps();
        void save_dirty_controlmaps();
        bool read_height(i32 world_x, i32 world_y, u8 &height);
        bool write_height(i32 world_x, i32 world_y, i32 height,
                          std::set<u32> &touched_chunks);
        bool write_controlmap(i32 world_x, i32 world_y, f32 amount,
                              std::set<u32> &touched_chunks);
        bool apply_raise_brush(i32 world_x, i32 world_y, f32 amount,
                               std::set<u32> &touched_chunks);
        bool apply_lower_brush(i32 world_x, i32 world_y, f32 amount,
                               std::set<u32> &touched_chunks);
        bool apply_flatten_brush(i32 world_x, i32 world_y, f32 amount,
                                 std::set<u32> &touched_chunks);
        bool apply_smooth_brush(i32 world_x, i32 world_y, f32 amount,
                                std::set<u32> &touched_chunks);
        bool apply_splat_brush(i32 world_x, i32 world_y, f32 amount,
                               std::set<u32> &touched_chunks);
        bool apply_brush_sample(i32 world_x, i32 world_y, f32 amount,
                                std::set<u32> &touched_chunks);
        void upload_touched_tiles(const std::set<u32> &touched_chunks);
        void draw_empty_viewport(ImVec2 origin, f32 viewport_w, f32 viewport_h,
                                 const char *message);
        i32 find_chunk_index_at_chunk(i32 chunk_x, i32 chunk_y) const;
        bool chunk_exists_at(i32 chunk_x, i32 chunk_y) const;
        bool add_chunk_at(i32 chunk_x, i32 chunk_y);
        i32 find_chunk_index_at_world(i32 world_x, i32 world_y) const;
        bool world_to_heightmap_pixel(i32 world_x, i32 world_y, u32 &chunk_idx,
                                      u32 &pixel_x, u32 &pixel_y) const;
        bool sample_terrain_pick_pixel(u32 x, u32 y, u32 viewport_h,
                                       i32 &world_x, i32 &world_y);
        bool sample_terrain_pick(ImVec2 viewport_origin, f32 viewport_w,
                                 f32 viewport_h, i32 &world_x, i32 &world_y);
        bool find_nearest_terrain_pixel(u32 start_x, u32 start_y,
                                        u32 viewport_w, u32 viewport_h,
                                        i32 &world_x, i32 &world_y, u32 &hit_x,
                                        u32 &hit_y);
        bool add_chunk_from_empty_viewport_click(ImVec2 viewport_origin,
                                                 f32 viewport_w,
                                                 f32 viewport_h);
        const char *terrain_tool_name(WorldTerrainTool tool) const;
        bool is_texture_asset(UUID uuid) const;
        std::string terrain_texture_label(UUID uuid) const;
        const char *terrain_palette_map_label(TerrainPaletteMap map) const;
        UUID terrain_palette_uuid(u32 index, TerrainPaletteMap map) const;
        const EditorUI::TexturePreview *terrain_palette_preview(
            u32 index, TerrainPaletteMap map) const;
        bool set_terrain_palette_texture(u32 index, TerrainPaletteMap map,
                                         UUID texture);
        bool accept_terrain_palette_drop(u32 index, TerrainPaletteMap map);
        bool is_static_model_asset(UUID uuid) const;
        std::string static_model_label(UUID uuid) const;
        void select_static_object(u32 chunk_index, u32 object_index);
        bool add_static_model(UUID uuid, i32 x, i32 y, i32 z);
        bool accept_static_model_drop(i32 x, i32 y, i32 z);
        void accept_static_model_drop_on_viewport(ImVec2 viewport_origin,
                                                  f32 viewport_w,
                                                  f32 viewport_h);
        void draw_terrain_palette_tooltip(
            UUID uuid, const EditorUI::TexturePreview *preview);
        void draw_terrain_palette_map(u32 index, TerrainPaletteMap map);
        void remove_terrain_palette_slot(u32 index);
        void draw_left_panel();
        void draw_center_panel();
        void draw_viewport(f32 viewport_w, f32 viewport_h);
        void edit_terrain_viewport(ImVec2 viewport_origin, f32 viewport_w,
                                   f32 viewport_h);
        void draw_right_panel();
        void draw_world_panel();
        void draw_terrain_panel();
        void draw_terrain_palette();
        void add_chunk();
        void clear_tiles();
        void draw_clear_tiles_confirmation_popup();
        void sync_tile_seams(std::set<u32> &touched_chunks);
        void apply_terrain_brush(i32 world_x, i32 world_y);
        void save_current_world();

    public:
        void init();
        ~WorldEditor();
        void update();
        bool load_world(const Path &path);
        EditorWorld *get_current_world() { return current_world; }
};

}  // namespace Seed

#endif
