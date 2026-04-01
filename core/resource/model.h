#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_

#include "core/ref.h"
#include "core/resource/animation.h"
#include "core/resource/resource.h"
#include "core/rendering/mesh.h"
#include "core/rendering/instance_data.h"
#include "core/resource/skeleton.h"
#include "core/transform.h"
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

class Model : public Resource {
        std::vector<Ref<Mesh>> meshes;

    public:
        Model(std::vector<Ref<Mesh>> &meshes) : meshes(std::move(meshes)) {}
        std::vector<Ref<Mesh>> &get_meshes() { return meshes; }
        virtual Ref<InstanceData> create_instance() = 0;
        virtual ~Model() = default;
};

template <typename Derived>
class ModelBase : public Model {
    public:
        ModelBase<Derived>(std::vector<Ref<Mesh>> &meshes) : Model(meshes) {}
        template <typename... Args>
        void add_instance(Ref<InstanceData> data, Args &&...args) {
            static_cast<Derived *>(this)->_add_instance(
                data, std::forward<Args>(args)...);
        }
};

class BasicModel : public ModelBase<BasicModel> {
        friend ResourceLoader;

    public:
        BasicModel(std::vector<Ref<Mesh>> &meshes)
            : ModelBase<BasicModel>(meshes) {}
        void _add_instance(Ref<InstanceData> data, Transform &transform) {
            Ref<StaticInstanceData> tdata = ref_cast<StaticInstanceData>(data);
            tdata->insert_transform(transform);
        };
        Ref<InstanceData> create_instance() override {
            Ref<StaticInstanceData> instance;
            instance.create();
            return ref_cast<InstanceData>(instance);
        };
        ~BasicModel() = default;
};

class SkeletonModel : public ModelBase<SkeletonModel> {
        friend ResourceLoader;

    private:
        Ref<Skeleton> skeleton;
        std::vector<Ref<Animation>> animations;

    public:
        SkeletonModel(std::vector<Ref<Mesh>> &meshes, Ref<Skeleton> skeleton)
            : ModelBase<SkeletonModel>(meshes), skeleton(skeleton) {}
        std::vector<Ref<Animation>> &get_animations() { return animations; }
        void add_animation(Ref<Animation> animation) {
            this->animations.push_back(animation);
        }
        void _add_instance(Ref<InstanceData> data, Transform &transform,
                           Ref<AnimationState> state) {
            Ref<SkeletonInstanceData> sdata =
                ref_cast<SkeletonInstanceData>(data);
            sdata->insert_instance(transform, state);
        };

        Ref<InstanceData> create_instance() override {
            Ref<SkeletonInstanceData> instance;
            instance.create(skeleton);
            return ref_cast<InstanceData>(instance);
        };
        ~SkeletonModel() = default;
};

}  // namespace Seed

#endif