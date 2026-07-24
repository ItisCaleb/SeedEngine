#ifndef _SEED_EDITOR_WORLD_H_
#define _SEED_EDITOR_WORLD_H_

#include <vector>

#include "core/misc/uuid.h"
#include "core/resource/world_setting.h"
#include "core/types.h"
#include "core/world/sky.h"
#include "editor/world/editor_terrain.h"
#include "editor/world/static_object.h"

namespace Seed {

class ResourceEntry;
class World;

enum class TerrainTextureKind { Diffuse, Normal };

class EditorWorld {
    private:
        World *world;
        ResourceEntry *entry;
        Ref<EditorTerrain> terrain;
        std::vector<StaticObject> objects;

        void load_terrain();
        void upload_terrain_palette();

    public:
        EditorWorld(World &world, ResourceEntry *entry);
        ~EditorWorld() = default;

        void reload();
        void save() const;

        void apply_directional_light();
        bool update_skybox_face(UUID uuid, CubemapFace face);
        bool update_terrain_texture(u32 layer, UUID uuid,
                                    TerrainTextureKind kind);
        void rebuild_static_models();
        bool terrain_chunk_exists_at(i32 x, i32 y) const;
        bool has_dirty_terrain_maps() const;
        void save_dirty_terrain_maps();

        WorldSetting &get_setting();
        const WorldSetting &get_setting() const;
        EditorTerrain *get_terrain() { return *terrain; }
        std::vector<ChunkSetting> &get_chunks();
        std::vector<StaticObject> &get_static_objects() { return objects; }
        const std::vector<ChunkSetting> &get_chunks() const;
        bool add_new_chunk(i32 x, i32 y);
        void clear_tiles();
};

}  // namespace Seed

#endif
