#ifndef _SEED_TERRAIN_H_
#define _SEED_TERRAIN_H_
#include "core/ref.h"
#include "core/math/vec2.h"
#include "core/resource/material.h"
#include "core/resource/image.h"
#include "core/physic/physic_body.h"
#include "core/collision/shape.h"
#include <vector>
#include "core/rendering/mesh.h"
#include "core/rendering/instance_data.h"
#include "core/resource/texture.h"

namespace Seed {

struct TerrainVertex {
        Vec2 pos;
};

struct TerrainInstance {
        Vec2 pos;
        u32 heightmap_index;
        f32 max_height, min_height;
};

class TerrainMaterial : public Material {
    private:
        Ref<TextureArray> textures;
        Ref<TextureArray> texture_normals;

    public:
        TerrainMaterial(Ref<Shader> shader, Ref<TextureArray> heightmaps,
                        Ref<TextureArray> controlmaps,
                        Ref<TextureArray> textures,
                        Ref<TextureArray> texture_normals);
        TerrainMaterial(Ref<TextureArray> heightmaps,
                        Ref<TextureArray> controlmaps,
                        Ref<TextureArray> textures,
                        Ref<TextureArray> texture_normals);
        Ref<TextureArray> get_textures() { return textures; }
        Ref<TextureArray> get_texture_normals() { return texture_normals; }
};

class TerrainInstanceData : public InstanceData {
    private:
        std::vector<TerrainInstance> instances;

    public:
        void insert_terrain_data(const TerrainInstance &instance);
        u32 size() override { return instances.size(); }
        void upload() override;

        void frustum_culling(const Frustum &frustum, const AABB &bounding_box,
                             std::vector<u32> &instance_ids,
                             std::vector<f32> &depths) override;
        void clear() override { instances.clear(); }
        TerrainInstanceData();
};

class Terrain : public RefCounted {
    private:
        u32 last_heightmap = 0;

        std::vector<PhysicBody> bodies;
        Ref<TextureArray> heightmaps;
        Ref<TextureArray> controlmaps;
        Ref<TextureArray> textures;
        Ref<TextureArray> texture_normals;

        Ref<TerrainMaterial> material;
        Ref<Mesh> mesh;

        Ref<TerrainInstanceData> instances;
        void build_mesh();

    public:
        Terrain();
        Ref<TerrainMaterial> get_material() { return material; }
        Ref<TerrainInstanceData> get_instance() { return instances; }
        Ref<Mesh> get_mesh() { return mesh; }
        void add_chunk(i32 x, i32 y, Ref<Image> height_map,
                       Ref<Image> control_map);
        ~Terrain();
};

}  // namespace Seed

#endif