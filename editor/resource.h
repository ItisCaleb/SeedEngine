#ifndef _EDITOR_RESOURCE_H_
#define _EDITOR_RESOURCE_H_
#include "core/rendering/mesh.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "core/collision/shape.h"
#include <string>
#include <vector>
#include <map>
#include "core/gui/gui.h"
#include "core/resource/model.h"
#include "core/resource/skeleton_model.h"

struct EditorMesh {
        std::vector<Seed::ModelVertex> vertices;
        std::vector<u32> indices;
        i16 material_id = -1;
};

struct AnimationMesh {
        std::vector<Seed::SkeletonVertex> vertices;
        std::vector<u32> indices;
        i16 material_id = -1;
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

// class Model {
//     protected:
// };

class DefaultModel {
    private:
        void processNode(aiNode *node, const aiScene *scene);
        void processMesh(aiMesh *mesh, const aiScene *scene);
        i16 loadMaterialTextures(aiMaterial *mat, aiTextureType type);
        Seed::AABB calculateAABB(
            const std::vector<Seed::ModelVertex> &vertices);

    public:
        std::string directory;
        std::vector<EditorMesh> meshes;
        std::vector<std::string> textures;
        std::vector<::Material> materials;

        void dump();
        void dump(const std::string &file_path);

        DefaultModel(const std::string &path);
};

class AnimationModel {};

class ModelGUI : public Seed::GUI {
        ::DefaultModel *current_model = nullptr;

    public:
        void update() override;
};

#endif