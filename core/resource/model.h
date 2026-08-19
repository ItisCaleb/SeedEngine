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

        virtual ~Model() = default;
};

class BasicModel : public Model {
        friend ResourceLoader;

    public:
        BasicModel(std::vector<Ref<Mesh>> &meshes) : Model(meshes) {}

        ~BasicModel() = default;
};

class SkeletonModel : public Model {
        friend ResourceLoader;

    private:
        Ref<Skeleton> skeleton;
        std::vector<Ref<Animation>> animations;

    public:
        SkeletonModel(std::vector<Ref<Mesh>> &meshes, Ref<Skeleton> skeleton)
            : Model(meshes), skeleton(skeleton) {}
        std::vector<Ref<Animation>> &get_animations() { return animations; }
        void add_animation(Ref<Animation> animation) {
            this->animations.push_back(animation);
        }
        Ref<Skeleton> get_skeleton() const { return skeleton; }

        ~SkeletonModel() = default;
};

}  // namespace Seed

#endif
