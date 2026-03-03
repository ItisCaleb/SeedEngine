#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_
#include "core/rendering/rhi/render_resource.h"
#include "core/collision/shape.h"
#include "core/resource/resource.h"
#include "core/rendering/mesh.h"
#include "core/rendering/instance_data.h"
#include <vector>

namespace Seed {

class RenderEngine;
class DefaultRenderer;

struct ModelVertex {
        Vec3 position;
        Vec3 normal;
        Vec3 tangent;
        Vec2 tex_coord;
};

struct SkeletonVertex {
        Vec3 position;
        Vec3 normal;
        Vec3 tangent;
        Vec2 tex_coord;
        u32 bone_ids[4];
        f32 bond_weights[4];
};

class Model : public Resource {
        friend RenderEngine;
        friend DefaultRenderer;

    private:
        std::vector<Ref<Mesh>> meshes;
        Ref<TransformInstanceData> instances;

    public:
        std::vector<Ref<Mesh>> &get_meshes() { return meshes; }
        Model(const std::vector<Ref<Mesh>> &meshes);
        void insert_transform(Ref<Transform> transform);
        void remove_transform(Ref<Transform> transform);
        ~Model();
};
}  // namespace Seed

#endif