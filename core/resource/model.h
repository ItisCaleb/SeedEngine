#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_

#include "core/resource/resource.h"
#include "core/rendering/mesh.h"
#include "core/rendering/instance_data.h"
#include "core/resource/skeleton.h"
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
        u16 bone_ids[4];
        f32 bone_weights[4];
};

class ResourceLoader;
class Model : public Resource {
        friend RenderEngine;
        friend DefaultRenderer;
        friend ResourceLoader;

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

class SkeletonModel : public Resource {
        friend ResourceLoader;

    private:
        std::vector<Ref<Mesh>> meshes;
        Ref<SkeletonInstanceData> instances;
        Ref<Skeleton> skeleton;
        std::vector<Ref<Animation>> animations;

    public:
        std::vector<Ref<Mesh>> &get_meshes() { return meshes; }
        std::vector<Ref<Animation>> &get_animations() { return animations; }
        void add_animation(Ref<Animation> animation) {
            this->animations.push_back(animation);
        }
        SkeletonModel(const std::vector<Ref<Mesh>> &meshes,
                      Ref<Skeleton> skeleton);
        void insert_instance(Ref<Transform> transform,
                             Ref<AnimationState> state);
        ~SkeletonModel();
};

}  // namespace Seed

#endif