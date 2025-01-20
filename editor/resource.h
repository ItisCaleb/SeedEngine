#ifndef _EDITOR_RESOURCE_H_
#define _EDITOR_RESOURCE_H_
#include "core/rendering/mesh.h"
#include <string>
#include <vector>
#include <map>

struct Mesh{
    std::vector<Seed::Vertex> vertices;
    std::vector<u32> indices;
    u32 diffuse_map = 0;
    u32 specular_map = 0;
    u32 normal_map = 0;
};

struct Model{
    std::string directory;
    std::vector<::Mesh> meshes;
    std::map<std::string, u8> textures;
    void dump();
    void dump(const std::string &file_path);
};

Model* parseModel(const std::string &path);

#endif