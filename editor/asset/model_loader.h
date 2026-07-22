#ifndef _SEED_MODEL_LOADER_H_
#define _SEED_MODEL_LOADER_H_

#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <assimp/scene.h>
#include "core/io/file.h"
#include "core/io/path.h"
#include "core/rendering/mesh.h"
#include "core/collision/shape.h"
#include "core/resource/model.h"
#include "core/resource/animation.h"

namespace Seed {
class ResourceConfiguration;

struct EditorMesh {
        std::vector<Seed::ModelVertex> vertices;
        std::vector<u32> indices;
        i16 material_id = -1;
};

struct EditorBoneMesh {
        std::vector<Seed::SkeletonVertex> vertices;
        std::vector<u32> indices;
        i16 material_id = -1;
};

struct Bone {
        std::string name;
        u16 parent;
        Seed::Mat4 offset_matrix;
};

struct EditorAnimation {
        std::string name;
        f32 duration;
        std::vector<Seed::AnimationClip> clips;
};

struct EditorMaterial {
        std::string name;
        std::vector<std::string> texture_names;
        f32 opacity;
};

class EditorModel {
    private:
        void processNode(aiNode *node, const aiScene *scene);
        void processMesh(aiMesh *mesh, const aiScene *scene);
        void processBoneMesh(aiMesh *mesh, const aiScene *scene);
        void processAnimation(const aiScene *scene);
        void processBoneHierachy(aiNode *node, const aiScene *scene,
                                 u16 parent_id);
        void collectBoneNames(aiNode *node, const aiScene *scene);
        Seed::AABB calculateAABB(
            const std::vector<Seed::ModelVertex> &vertices);
        Seed::AABB calculateAABB(
            const std::vector<Seed::SkeletonVertex> &vertices);
        i16 get_bone_id(const std::string &name);
        std::set<std::string> bone_names;
        std::unordered_map<std::string, u16> bone_map;

    public:
        std::vector<EditorMesh> meshes;
        std::vector<EditorBoneMesh> bone_meshes;
        std::vector<Bone> bones;
        std::vector<EditorAnimation> animations;
        std::vector<EditorMaterial> materials;

        void dump(ResourceConfiguration &conf, Ref<File> bin_f);

        EditorModel(const Seed::Path &path);
};
}  // namespace Seed

#endif
