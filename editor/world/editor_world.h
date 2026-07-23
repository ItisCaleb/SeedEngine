#ifndef _SEED_EDITOR_WORLD_H_
#define _SEED_EDITOR_WORLD_H_

#include <vector>

#include "core/misc/uuid.h"
#include "core/resource/world_setting.h"
#include "core/types.h"
#include "editor_terrain.h"
#include "editor_world_state.h"

namespace Seed {

class ResourceEntry;

enum class TerrainTextureKind { Diffuse, Normal };

class EditorWorld {
    private:
        EditorWorldState state;
        Ref<WorldSetting> setting;
        Ref<EditorTerrain> terrain;

        void upload_terrain_palette_to_gpu();

    public:
        EditorWorld(ResourceEntry *entry);
        ~EditorWorld() = default;

        void reload();
        void save();
        void apply_directional_light();
        void update_skybox_face(UUID uuid, CubemapFace face);
        bool update_terrain_texture(u32 layer, UUID uuid,
                                    TerrainTextureKind kind);
        void rebuild_static_models();
        bool terrain_chunk_exists_at(i32 x, i32 y) const;
        bool has_dirty_terrain_maps() const;
        void save_dirty_terrain_maps();

        WorldSetting *get_setting() { return *setting; }
        EditorSky &get_sky() { return state.get_sky(); }
        EditorTerrain *get_terrain() { return *terrain; }
        std::vector<ChunkSetting> &get_chunks() { return setting->chunks; }
        const std::vector<ChunkSetting> &get_chunks() const {
            return setting->chunks;
        }
        bool add_new_chunk(i32 x, i32 y);
        void clear_tiles();
        const std::map<UUID, EditorStaticModel> &get_static_models() const {
            return state.get_static_models();
        }
};

}  // namespace Seed

#endif
