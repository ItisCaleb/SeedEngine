#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_

#include "core/ref.h"
#include "core/resource/animation.h"
#include "core/resource/resource.h"
#include "core/rendering/mesh.h"
#include "core/rendering/instance_batch.h"
#include "core/resource/skeleton.h"
#include "core/transform.h"
#include <utility>
#include <vector>

namespace Seed {

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

/*
 * Instance data usage:
 *
 *   Ref<InstanceBatch> data = model->create_instance();
 *   mesh_storage->add_model(ref_cast<Model>(model), data);
 *
 *   data->clear();
 *   model->add_instance(data, transform);          // BasicModel
 *   model->add_instance(data, transform, state);   // SkeletonModel
 *
 * create_instance() creates one batch for a model in a world or other render
 * group. It should not be called once per visible object. All meshes belonging
 * to the model share that batch, and the renderer uploads it automatically
 * after the batch has been registered with MeshStorage.
 */
class Model : public Resource {
        std::vector<Ref<Mesh>> meshes;

    public:
        Model(std::vector<Ref<Mesh>> &meshes) : meshes(std::move(meshes)) {}
        std::vector<Ref<Mesh>> &get_meshes() { return meshes; }

        /*
         * Create an empty InstanceBatch subtype compatible with this model.
         * This only creates the batch; it does not register, populate, or
         * upload it.
         */
        virtual Ref<InstanceBatch> create_instance() = 0;
        virtual ~Model() = default;
};

template <typename Derived>
class ModelBase : public Model {
    public:
        ModelBase<Derived>(std::vector<Ref<Mesh>> &meshes) : Model(meshes) {}

        /*
         * Forward one logical instance to the derived model's _add_instance()
         * implementation. Arguments differ by model type.
         */
        template <typename... Args>
        void add_instance(Ref<InstanceBatch> data, Args &&...args) {
            static_cast<Derived *>(this)->_add_instance(
                data, std::forward<Args>(args)...);
        }
};

class BasicModel : public ModelBase<BasicModel> {
        friend ResourceLoader;

    public:
        BasicModel(std::vector<Ref<Mesh>> &meshes)
            : ModelBase<BasicModel>(meshes) {}

        /*
         * Append one transform to the StaticInstanceBatch returned by
         * create_instance(). GPU upload is deferred to the renderer.
         */
        void _add_instance(Ref<InstanceBatch> data, Transform &transform) {
            Ref<StaticInstanceBatch> tdata = ref_cast<StaticInstanceBatch>(data);
            tdata->insert_transform(transform);
        };

        /* Create the empty transform batch used by all meshes in this model. */
        Ref<InstanceBatch> create_instance() override {
            Ref<StaticInstanceBatch> instance;
            instance.create();
            return ref_cast<InstanceBatch>(instance);
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

        /*
         * Append one transform and animation pose to the
         * SkeletonInstanceBatch returned by create_instance(). A null state
         * inserts the bind/default pose. GPU upload is deferred.
         */
        void _add_instance(Ref<InstanceBatch> data, Transform &transform,
                           AnimationState *state) {
            Ref<SkeletonInstanceBatch> sdata =
                ref_cast<SkeletonInstanceBatch>(data);
            sdata->insert_instance(transform, state);
        };

        /*
         * Create an empty skeletal batch. Each logical instance occupies one
         * world transform followed by one matrix per skeleton bone.
         */
        Ref<InstanceBatch> create_instance() override {
            Ref<SkeletonInstanceBatch> instance;
            instance.create(skeleton);
            return ref_cast<InstanceBatch>(instance);
        };
        ~SkeletonModel() = default;
};

}  // namespace Seed

#endif
