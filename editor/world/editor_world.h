#ifndef _SEED_EDITOR_WORLD_H_
#define _SEED_EDITOR_WORLD_H_

#include <map>
#include <nlohmann/json.hpp>
#include <vector>
#include "core/container/kstring.h"
#include "core/misc/uuid.h"
#include "core/resource/image.h"
#include "core/resource/model.h"
#include "core/resource/resource_entry.h"
#include "core/resource/texture.h"
#include "core/resource/world_setting.h"
#include "core/types.h"
#include "core/world/sky.h"
#include "editor/gui/editor_ui.h"
#include "editor/gui/inspectable.h"
#include "editor_terrain.h"
#include "world_renderer.h"

namespace Seed {

struct EditorSky : public SkySetting {
        Ref<TextureCubemap> cubemap;
        Ref<Sky> sky;
};

struct EditorStaticModel {
        Ref<BasicModel> model;
        Ref<InstanceData> instance;
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

        Ref<EditorTerrain> terrain;
        std::vector<EditorUI::TexturePreview> terrain_texture_previews;
        std::vector<EditorUI::TexturePreview> terrain_normal_previews;
        std::map<std::pair<u32, u32>, EditorStaticModel> static_models;

        void copy_sky_setting_to_editor(const SkySetting &setting);
        void copy_editor_sky_to_setting(SkySetting &setting);
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
        void rebuild_static_model_instances();
        bool update_static_model_instance(u32 chunk_index, u32 object_index);
        bool terrain_chunk_exists_at(i32 x, i32 y) const;
        bool has_dirty_terrain_maps() const;
        void save_dirty_terrain_maps();

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
        bool add_new_chunk(i32 x, i32 y);
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
        std::map<std::pair<u32, u32>, EditorStaticModel> &get_static_models() {
            return static_models;
        }
};

class EditorWorldInspector : public Inspectable {
    private:
        EditorWorld *world;

    public:
        EditorWorldInspector(EditorWorld *world);
        virtual void draw_inspector() override;
        virtual void save() override;
};

class EditorStaticObjectInspector : public Inspectable {
    private:
        EditorWorld *world;
        u32 chunk_index;
        u32 object_index;

    public:
        EditorStaticObjectInspector(EditorWorld *world, u32 chunk_index,
                                    u32 object_index);
        virtual void draw_inspector() override;
        virtual void save() override;
};

}  // namespace Seed

#endif
