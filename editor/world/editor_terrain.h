#ifndef _SEED_EDITOR_TERRAIN_H_
#define _SEED_EDITOR_TERRAIN_H_

#include <set>
#include <vector>

#include "core/ref.h"
#include "core/resource/image.h"
#include "core/resource/resource.h"
#include "core/resource/texture.h"
#include "core/resource/world_setting.h"
#include "core/world/terrain.h"
#include "terrain_brush.h"

namespace Seed {

class EditorTerrain : public Resource {
    private:
        Ref<Mesh> mesh;
        Ref<TextureArray> heightmaps;
        Ref<TextureArray> controlmaps;
        Ref<TextureArray> textures;
        Ref<TextureArray> texture_normals;
        Ref<Image> fallback_texture;
        Ref<Image> fallback_normal;

        Ref<TerrainMaterial> material;
        Ref<TerrainInstanceData> instances;

        TerrainTileMap tile_map;

        void build_mesh();
        void upload_fallback_layer(u32 layer);
        void commit_edit(TerrainEdit &edit);
        void upload_heightmaps(const std::set<u32> &touched_chunks);
        void upload_controlmaps(const std::set<u32> &touched_chunks);

    public:
        EditorTerrain();
        void add_chunk(i32 x, i32 y, Ref<Image> height_map,
                       Ref<Image> control_map);
        void clear_chunks();
        void connect_chunk(u32 chunk_index);
        void sync_loaded_tile_seams();
        bool update_texture_layer(u32 layer, RHI::UpdateBufferInfo info);
        bool update_normal_layer(u32 layer, RHI::UpdateBufferInfo info);
        void reset_texture_palette();

        bool chunk_exists_at(i32 x, i32 y) const;
        bool read_height(i32 world_x, i32 world_y, u8 &height);
        bool apply_brush(i32 x, i32 y, TerrainBrush type,
                         const TerrainBrushSetting &setting);

        bool has_dirty_maps() const { return tile_map.has_dirty_maps(); }
        void clear_dirty_maps();
        void save_dirty_maps(const std::vector<ChunkSetting> &chunks);

        Ref<Mesh> get_mesh() { return mesh; }
        Ref<TerrainInstanceData> get_instances() { return instances; }
        ~EditorTerrain() = default;
};
}  // namespace Seed

#endif
