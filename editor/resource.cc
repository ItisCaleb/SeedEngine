#include "resource.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <spdlog/spdlog.h>
#include "core/io/file.h"
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace Seed;

static u8 loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                                Model &model, ::Mesh &mesh) {
    std::vector<std::string> textures;
    for (int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        auto it = model.textures.find(str.C_Str());
        if (it == model.textures.end()) {
            u8 id = model.textures.size() + 1;
            model.textures[str.C_Str()] = id;
            return id;
        }
        return it->second;
    }
    return 0;
}

static void processMesh(aiMesh *mesh, const aiScene *scene, Model &model) {
    model.meshes.push_back({});
    ::Mesh &m = model.meshes.back();
    std::vector<Vertex> &vertices = m.vertices;
    std::vector<u32> &indices = m.indices;
    for (int i = 0; i < mesh->mNumVertices; i++) {
        aiVector3D ai_vertex = mesh->mVertices[i];
        aiVector3D ai_normal = mesh->mNormals[i];
        aiVector3D *ai_tex_coord = mesh->mTextureCoords[0];
        Vertex vertex;
        vertex.position = {ai_vertex.x, ai_vertex.y, ai_vertex.z};
        vertex.normal = {ai_normal.x, ai_normal.y, ai_normal.z};
        if (ai_tex_coord) {
            vertex.tex_coord = {ai_tex_coord[i].x, ai_tex_coord[i].y};
        } else {
            vertex.tex_coord = {0, 0};
        }
        vertices.push_back(vertex);
    }
    for (int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
        m.diffuse_map =
            loadMaterialTextures(mat, aiTextureType_DIFFUSE, model, m);
        m.specular_map =
            loadMaterialTextures(mat, aiTextureType_SPECULAR, model, m);
    }
}

static void processNode(aiNode *node, const aiScene *scene, Model &model) {
    for (int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, model);
    }
    for (int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, model);
    }
}

Model *parseModel(const std::string &path) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                  aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    // If the import failed, report it
    if (!scene) {
        spdlog::error("Can't load Model from {}", path);
        return {};
    }
    Model *model = new Model;
    processNode(scene->mRootNode, scene, *model);
    fmt::println("{}", scene->mNumMeshes);

    std::filesystem::path dir = path;
    model->directory = dir.parent_path().string();
    return model;
}

void Model::dump() {
    std::string path = fmt::format("{}/test.mdl", this->directory);
    dump(path);
}

struct TextureField{
    u8 path_length;
    u8 id;
    char path[];
};

struct ModelHeader{
    u16 mesh_count;
    u16 texture_count;
    TextureField textures[];
};

struct MeshHeader{
    u32 vertex_size;
    u32 index_size;
    u8 diffuse_map;
    u8 specular_map;
    u8 normal_map;
    u8 unused;
    u32 next_mesh;
};

void Model::dump(const std::string &file_path) {
    Ref<File> f = File::open(file_path, "w");
    ModelHeader header = {(u16)this->meshes.size(), (u16)this->textures.size()};
    const char *magic = "SDMDL";
    u32 total_size = strlen(magic) + sizeof(ModelHeader);
    f->write((void *)magic, strlen(magic));
    f->write(&header, sizeof(ModelHeader));
    for(auto &[texture_path, id]: this->textures){
        TextureField field = {(u8)texture_path.length(), id};
        f->write(&field, sizeof(TextureField));
        f->write((void*)texture_path.data(), field.path_length);
        total_size += sizeof(TextureField) + field.path_length;
    }

    /*
    ----------------------------------------------
    |         | model  |        |          |
    | "SDMDL" |        | header | vertices | indices
    |         | header |        |          |
    ----------------------------------------------
    */
    for (auto &mesh : this->meshes) {
        total_size += sizeof(MeshHeader);
        total_size += mesh.vertices.size() * sizeof(Vertex);
        total_size += mesh.vertices.size() * sizeof(u32);
        MeshHeader header;
        header.vertex_size = mesh.vertices.size();
        header.index_size = mesh.indices.size();
        header.diffuse_map = mesh.diffuse_map;
        header.specular_map = mesh.specular_map;
        header.normal_map = mesh.normal_map;
        header.next_mesh = total_size;
        f->write(&header, sizeof(MeshHeader));
        f->write(mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
        f->write(mesh.indices.data(), mesh.indices.size() * sizeof(u32));
    }
    fmt::println("model size:{}",total_size);
    // stbi_set_flip_vertically_on_load(true);

    // for (auto &[texture_path, id] : this->textures) {
    //     std::string final_path =
    //         fmt::format("{}/{}", this->directory, texture_path);
    //     int w, h, comp;
    //     void *data = stbi_load(final_path.c_str(), &w, &h, &comp, 4);
    //     f->write(data, w * h * comp);
    //     stbi_image_free(data);
    // }
}