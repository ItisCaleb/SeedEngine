#include "resource.h"

#include <assimp/postprocess.h> 
#include <assimp/scene.h>     
#include <assimp/Importer.hpp>
#include <spdlog/spdlog.h> 

using namespace Seed;
Ref<Mesh> parseMesh(const std::string &path){
    Assimp::Importer importer;
    Ref<Mesh> mesh;
    const aiScene *scene = importer.ReadFile(
        path, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                  aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    // If the import failed, report it
    if (!scene) {
        spdlog::error("Can't load Mesh from {}", path);
        return mesh;
    }
}