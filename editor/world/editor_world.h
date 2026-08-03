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

class EditorWorld : public RefCounted {
    private:
        ResourceEntry *entry;
        Ref<EditorTerrain> terrain;
        std::vector<StaticObject> objects;
        World *world;

        World* get_world() const;

    public:
        EditorWorld(ResourceEntry *entry);
        ~EditorWorld() = default;

        void register_editor_components();

        void reload();
        void save() const;

        void apply_directional_light();
        bool update_skybox_face(UUID uuid, CubemapFace face);
        bool update_terrain_texture(u32 layer, UUID uuid,
                                    TerrainTextureKind kind);
        void rebuild_static_models();
        bool terrain_chunk_exists_at(i32 x, i32 y) const;
        void save_dirty_terrain_maps();
        // void add_static_object(const Transform  );

        Ref<WorldSetting> get_setting() const;
        Ref<EditorTerrain> get_terrain() const { return terrain; }
        std::vector<ChunkSetting> &get_chunks();
        std::vector<StaticObject> &get_static_objects() { return objects; }
        bool add_new_chunk(i32 x, i32 y);
        void clear_tiles();
};

}  // namespace Seed

#endif
