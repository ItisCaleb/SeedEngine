#ifndef _SEED_EDITOR_WORLD_H_
#define _SEED_EDITOR_WORLD_H_

#include <map>
#include <vector>

#include "core/misc/uuid.h"
#include "core/resource/model.h"
#include "core/resource/texture.h"
#include "core/resource/world_setting.h"
#include "core/types.h"
#include "core/world/sky.h"
#include "editor_terrain.h"

namespace Seed {

class ResourceConfiguration;
class ResourceEntry;

struct EditorSky : public SkySetting {
        Ref<TextureCubemap> cubemap;
        Ref<Sky> sky;
};

struct EditorStaticModel {
        Ref<BasicModel> model;
        Ref<InstanceData> instance;
};

enum class TerrainTextureKind { Diffuse, Normal };

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
        std::map<std::pair<u32, u32>, EditorStaticModel> static_models;

        void copy_sky_setting_to_editor(const SkySetting &setting);
        void copy_editor_sky_to_setting(SkySetting &setting);
        void read_chunk_controlmaps_from_config();
        void read_terrain_textures_from_config();
        void normalize_terrain_palette_size();
        void upload_terrain_palette_to_gpu();

    public:
        EditorWorld(ResourceEntry *entry);
        ~EditorWorld() = default;

        void reload();
        void save();
        void apply_directional_light_to_runtime();
        void update_skybox_face(UUID uuid, CubemapFace face);
        bool update_terrain_texture(u32 layer, UUID uuid,
                                    TerrainTextureKind kind);
        void rebuild_static_model_instances();
        bool update_static_model_instance(u32 chunk_index, u32 object_index);
        bool terrain_chunk_exists_at(i32 x, i32 y) const;
        bool has_dirty_terrain_maps() const;
        void save_dirty_terrain_maps();

        WorldSetting *get_setting() { return *setting; }
        EditorSky &get_sky() { return sky; }
        std::vector<ChunkSetting> &get_chunks() { return setting->chunks; }
        const std::vector<ChunkSetting> &get_chunks() const {
            return setting->chunks;
        }
        bool add_new_chunk(i32 x, i32 y);
        void clear_tiles();
        std::map<std::pair<u32, u32>, EditorStaticModel> &get_static_models() {
            return static_models;
        }
};

}  // namespace Seed

#endif
