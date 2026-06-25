#ifndef _SEED_EDITOR_WORLD_H_
#define _SEED_EDITOR_WORLD_H_

#include <map>
#include <set>
#include <nlohmann/json.hpp>
#include <vector>
#include "core/container/kstring.h"
#include "core/math/vec3.h"
#include "core/misc/uuid.h"
#include "core/resource/image.h"
#include "core/resource/resource_entry.h"
#include "core/resource/sky.h"
#include "core/resource/texture.h"
#include "core/resource/world_setting.h"
#include "core/types.h"
#include "editor/gui/editor_ui.h"
#include "editor/gui/inspectable.h"
#include "editor_terrain.h"
#include "world_renderer.h"

namespace Seed {

struct EditorSky : public SkySetting {
        Ref<TextureCubemap> cubemap;
        Ref<Sky> sky;
};

class EditorTile {
    public:
        enum class TileDirection { UP, BOTTOM, RIGHT, LEFT };

    private:
        void clamp_border(Ref<Image> image);
        bool build_edge_from_image(Ref<Image> image, Ref<Image> source,
                                   TileDirection direction);
        bool build_border_from_image(Ref<Image> image, Ref<Image> source,
                                     TileDirection direction);

    public:
        Ref<Image> heightmap;
        Ref<Image> controlmap;

        void clamp_border();
        bool build_edge_from_tile(EditorTile *tile, TileDirection direction);
        bool build_border_from_tile(EditorTile *tile, TileDirection direction);
};

class WorldEditor;
class WorldRenderer;
class EditorWorld {
        friend WorldEditor;
        friend WorldRenderer;

    private:
        ResourceEntry *entry = nullptr;
        ResourceConfiguration *config = nullptr;
        Ref<WorldSetting> setting;
        EditorSky sky;
        std::map<std::pair<i32, i32>, u32> pos_to_index;
        std::map<u32, EditorTile> tiles;

        Ref<EditorTerrain> terrain;
        std::vector<EditorUI::TexturePreview> terrain_texture_previews;
        std::vector<EditorUI::TexturePreview> terrain_normal_previews;

        void copy_sky_setting_to_editor(const SkySetting &setting);
        void copy_editor_sky_to_setting(SkySetting &setting);
        i32 find_chunk_index_at(i32 x, i32 y);
        EditorTile *get_tile_at(i32 x, i32 y);
        void rebuild_tile_border(u32 chunk_index);
        void sync_tile_neighbor(u32 chunk_index, i32 neighbor_index,
                                EditorTile::TileDirection direction,
                                std::set<u32> &touched_chunks);
        void sync_loaded_tile_seams();
        void read_chunk_controlmaps_from_config();
        void read_terrain_textures_from_config();
        void normalize_terrain_palette_size();
        EditorUI::TexturePreview build_texture_preview(UUID texture_uuid);
        void rebuild_terrain_texture_previews();
        void upload_terrain_palette_to_gpu();

    public:
        EditorWorld(ResourceEntry *entry);
        ~EditorWorld() = default;

        void reload();
        void save();
        void apply_directional_light_to_runtime();
        void update_skybox_face(UUID uuid, CubemapFace face);
        void sync_tile_seams(std::set<u32> &touched_chunks);

        ResourceConfiguration *get_config() { return config; }
        const KString &get_name() const { return setting->name; }
        void set_name(KStr name) { setting->name = name; }
        EditorSky &get_sky() { return sky; }
        DirectionalLightSetting &get_directional_light() {
            return setting->dir_light;
        }
        std::vector<ChunkSetting> &get_chunks() { return setting->chunks; }
        const std::vector<ChunkSetting> &get_chunks() const {
            return setting->chunks;
        }
        void add_new_chunk(i32 x, i32 y);
        void clear_tiles();
        std::vector<UUID> &get_terrain_textures() {
            return setting->terrain_textures;
        }
        const std::vector<UUID> &get_terrain_textures() const {
            return setting->terrain_textures;
        }
        std::vector<UUID> &get_terrain_normals() {
            return setting->terrain_normals;
        }
        const std::vector<UUID> &get_terrain_normals() const {
            return setting->terrain_normals;
        }
        const EditorUI::TexturePreview *get_terrain_texture_preview(
            u32 index) const;
        const EditorUI::TexturePreview *get_terrain_normal_preview(
            u32 index) const;
        void add_terrain_texture(UUID texture);
        bool set_terrain_texture(u32 index, UUID texture);
        bool set_terrain_normal(u32 index, UUID texture);
        void remove_terrain_texture(u32 index);
        EditorTile *get_tile(u32 index);
};

class EditorWorldInspector : public Inspectable {
    private:
        EditorWorld *world;

    public:
        EditorWorldInspector(EditorWorld *world);
        virtual void draw_inspector() override;
        virtual void save() override;
};

}  // namespace Seed

#endif