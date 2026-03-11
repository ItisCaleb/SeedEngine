#ifndef _SEED_EDITOR_TERRAIN_H_
#define _SEED_EDITOR_TERRAIN_H_
#include "core/resource/terrain.h"
#include "core/resource/mappable_texture.h"

namespace Seed {
class EditorTerrainMaterial : public Material {
    public:
        EditorTerrainMaterial(Ref<Texture> height_map);
        Ref<Texture> get_height_map();
};

class EditorTerrain : public Resource {
    private:
        inline static VertexLayout *layout = nullptr;
        u32 width, depth;
        u32 hmap_width, hmap_height;
        Vec3 position;
        Ref<Mesh> mesh;
        Ref<MappableTexture> heightmap_texture;
        Ref<EditorTerrainMaterial> material;
        Ref<TerrainInstanceData> instances;
        void build_mesh();
        void create_chunk(Ref<Image> height_map, i32 left, i32 bottom,
                          u32 half_width, u32 half_depth);

    public:
        EditorTerrain(Ref<Image> height_map);
        Ref<MappableTexture> get_heightmap() { return heightmap_texture; }
        Ref<Mesh> get_mesh(){
            return mesh;
        }
        Ref<TerrainInstanceData> get_instances(){
            return instances;
        }
        u32 get_width(){
            return width;
        }
        u32 get_height(){
            return depth;
        }
        ~EditorTerrain();
};
}  // namespace Seed

#endif