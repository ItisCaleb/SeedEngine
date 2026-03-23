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
        Vec2 tex_coord;
};

struct TerrainInstance {
        Vec2 pos;
        Vec2 tex_coord;
        f32 max_height, min_height;
};

class TerrainMaterial : public Material {
    public:
        TerrainMaterial(Ref<Texture> height_map, Ref<Texture> light_map,
                        Ref<Texture> splat_map);
        void set_height_map(Ref<Texture> height_map);
        void set_light_map(Ref<Texture> light_map);
        Ref<Texture> get_height_map();
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
        TerrainInstanceData();
};

class Terrain : public Resource {
    private:
        u32 width, depth;
        u32 hmap_width, hmap_height;
        Vec3 position;
        std::vector<PhysicBody> bodies;
        Ref<TerrainMaterial> terrain_mat;
        Ref<Mesh> mesh;

        Ref<TerrainInstanceData> instances;
        void build_mesh();
        void create_chunk(Ref<Image> height_map, i32 left, i32 top,
                          u32 half_width, u32 half_depth);

    public:
        Terrain(Ref<Image> height_map, Ref<Texture> light_map,
                Ref<Texture> splat_map);
        void set_material(Ref<TerrainMaterial> mat) {
            this->mesh->set_material(ref_cast<Material>(mat));
            this->terrain_mat = mat;
        }
        Ref<TerrainMaterial> get_material() { return terrain_mat; }

        ~Terrain();
};

}  // namespace Seed

#endif