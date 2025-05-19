#ifndef _EDITOR_RESOURCE_H_
#define _EDITOR_RESOURCE_H_
#include "core/rendering/mesh.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "core/collision/aabb.h"
#include <string>
#include <vector>
#include <map>

struct Mesh {
        std::vector<Seed::Vertex> vertices;
        std::vector<u32> indices;
        i16 material_id = -1;
};

class Model {
    private:
        void processNode(aiNode *node, const aiScene *scene);
        void processMesh(aiMesh *mesh, const aiScene *scene);
        i16 loadMaterialTextures(aiMaterial *mat, aiTextureType type);
        void calculateAABB();

    public:
        struct Material {
                i16 diffuse = -1;
                i16 specular = -1;
                i16 normal = -1;
                bool operator==(Material &other) {
                    return diffuse == other.diffuse &&
                           specular == other.specular && normal == other.normal;
                }
                bool is_null() {
                    return diffuse == -1 && specular == -1 && normal == -1;
                }
        };
        std::string directory;
        std::vector<::Mesh> meshes;
        std::vector<std::string> textures;
        std::vector<Material> materials;
        Seed::AABB bounding_box;

        void dump();
        void dump(const std::string &file_path);

        Model(const std::string &path);
};

#endif