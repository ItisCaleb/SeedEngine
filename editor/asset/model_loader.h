#ifndef _SEED_MODEL_LOADER_H_
#define _SEED_MODEL_LOADER_H_

#include "core/rendering/mesh.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "core/collision/shape.h"
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "core/resource/model.h"
#include "core/resource/animation.h"

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
        float duration;
        std::vector<Seed::AnimationClip> clips;
};

struct Material {
        i16 diffuse = -1;
        i16 specular = -1;
        i16 normal = -1;
        f32 opacity = 1.0f;
        bool operator==(Material &other) {
            return diffuse == other.diffuse && specular == other.specular &&
                   normal == other.normal && opacity == other.opacity;
        }
        bool is_null() {
            return diffuse == -1 && specular == -1 && normal == -1 &&
                   opacity == 1.0f;
        }
};

class EditorModel {
    private:
        void processNode(aiNode *node, const aiScene *scene);
        void processMesh(aiMesh *mesh, const aiScene *scene);
        void processBoneMesh(aiMesh *mesh, const aiScene *scene);
        void processAnimation(const aiScene *scene);
        void processBoneHierachy(aiNode *node, const aiScene *scene, u16 parent_id);
        void collectBoneNames(aiNode *node, const aiScene *scene);
        i16 get_material_id(aiMesh *mesh, const aiScene *scene);
        i16 loadMaterialTextures(aiMaterial *mat, aiTextureType type);
        Seed::AABB calculateAABB(
            const std::vector<Seed::ModelVertex> &vertices);
        Seed::AABB calculateAABB(
            const std::vector<Seed::SkeletonVertex> &vertices);
        i16 get_bone_id(const std::string &name);
        std::set<std::string> bone_names;
        std::unordered_map<std::string, u16> bone_map;

    public:
        std::string origin_path;
        std::filesystem::path origin_dir;
        std::vector<EditorMesh> meshes;
        std::vector<EditorBoneMesh> bone_meshes;
        std::vector<std::string> textures;
        std::vector<Bone> bones;
        std::vector<EditorAnimation> animations;
        std::vector<::Material> materials;

        void dump(const std::string &file_path);

        EditorModel(const std::string &path);
};

#endif