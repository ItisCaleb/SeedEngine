#ifndef _SEED_EDITOR_TERRAIN_H_
#define _SEED_EDITOR_TERRAIN_H_
#include "core/ref.h"
#include "core/world/terrain.h"
#include "core/resource/texture.h"

namespace Seed {
class EditorTerrainMaterial : public Material {
    public:
        EditorTerrainMaterial(Ref<Texture> height_map);
        Ref<Texture> get_height_map();
};

class TerrainEditor;
class EditorTerrain : public Resource {
        friend TerrainEditor;

    private:
        u32 last_heightmap = 0;
        Ref<Mesh> mesh;
        Ref<TextureArray> heightmaps;
        Ref<EditorTerrainMaterial> material;
        Ref<TerrainInstanceData> instances;
        bool light_map_generated = false;
        void build_mesh();
        // void gen_lightmap();

    public:
        EditorTerrain();
        /* add a chunk at (x,y)*/
        /* x y is chunk position which will be multiplied by CHUNK SIZE */
        void add_chunk(i32 x, i32 y, Ref<Image> height_map);
        void clear_chunks();
        void update_chunk_heightmap(u32 chunk_index, Ref<Image> height_map);
        Ref<TextureArray> get_heightmap() { return heightmaps; }
        Ref<EditorTerrainMaterial> get_material() { return material;}
        Ref<Mesh> get_mesh() { return mesh; }
        Ref<TerrainInstanceData> get_instances() { return instances; }
        ~EditorTerrain();
};
}  // namespace Seed

#endif
