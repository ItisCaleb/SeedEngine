#include "resource.h"

#include <spdlog/spdlog.h>
#include "core/io/file.h"
#include <filesystem>
#include "core/rendering/model_file.h"
#include <algorithm>

using namespace Seed;

i16 Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type) {
    for (int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);

        /* find if texture exist */
        for (int j = 0; j < textures.size(); j++) {
            if (textures[j].compare(str.C_Str()) == 0) return j;
        }

        /* add new texture */
        textures.push_back(str.C_Str());
        return textures.size() - 1;
    }
    return -1;
}

void Model::calculateAABB() {
    f32 x1 = 1e5, x2 = -1e5;
    f32 y1 = 1e5, y2 = -1e5;
    f32 z1 = 1e5, z2 = -1e5;
    for (auto &mesh : this->meshes) {
        for (auto &vertex : mesh.vertices) {
            x1 = std::min(x1, vertex.position.x);
            x2 = std::max(x2, vertex.position.x);
            y1 = std::min(y1, vertex.position.y);
            y2 = std::max(y2, vertex.position.y);
            z1 = std::min(z1, vertex.position.z);
            z2 = std::max(z2, vertex.position.z);
        }
    }
    f32 w = (x2 - x1) / 2;
    f32 h = (y2 - y1) / 2;
    f32 d = (z2 - z1) / 2;
    this->bounding_box = {Vec3{x2 - w, y2 - h, z2 - d}, Vec3{w, h, d}};
}

void Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    meshes.push_back({});
    ::Mesh &m = meshes.back();
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
        Material model_mat;
        model_mat.diffuse = loadMaterialTextures(mat, aiTextureType_DIFFUSE);
        model_mat.specular = loadMaterialTextures(mat, aiTextureType_SPECULAR);
        model_mat.normal = loadMaterialTextures(mat, aiTextureType_NORMALS);
        if(model_mat.normal == -1){
            /* we try to load heightmap instead*/
            model_mat.normal = loadMaterialTextures(mat, aiTextureType_HEIGHT);
        }

        for (int i = 0; i < materials.size(); i++) {
            if (materials[i] == model_mat) {
                m.material_id = i;
                return;
            }
        }
        if (!model_mat.is_null()) {
            materials.push_back(model_mat);
            m.material_id = materials.size() - 1;
        }
    }
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    for (int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }
    for (int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Model::Model(const std::string &path) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                  aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices |
                  aiProcess_SortByPType);

    // If the import failed, report it
    if (!scene) {
        spdlog::error("Can't load Model from {}", path);
        return;
    }
    processNode(scene->mRootNode, scene);
    fmt::println("{}", scene->mNumMeshes);
    calculateAABB();
    std::filesystem::path dir = path;
    directory = dir.parent_path().string();
}

void Model::dump() {
    std::string path = fmt::format("{}/test.mdl", this->directory);
    dump(path);
}

void Model::dump(const std::string &file_path) {
    Ref<File> f = File::open(file_path, "wb");
    ModelHeader header;
    header.mesh_count = meshes.size();
    header.texture_count = textures.size();
    header.material_count = materials.size();
    header.bounding_box = this->bounding_box;
    /* calculate offsets */
    header.mesh_offset = strlen(model_file_magic) + sizeof(ModelHeader);
    header.texture_offset =
        header.mesh_offset + header.mesh_count * sizeof(MeshHeader);
    for (auto &mesh : this->meshes) {
        header.texture_offset += mesh.vertices.size() * sizeof(Vertex);
        header.texture_offset += mesh.indices.size() * sizeof(u32);
    }
    header.material_offset = header.texture_offset;
    for (auto &texture_path : this->textures) {
        header.material_offset += sizeof(TextureField);
        header.material_offset += texture_path.length();
    }
    fmt::println("mesh offset at: {:#x}", header.mesh_offset);
    fmt::println("texture offset at: {:#x}", header.texture_offset);
    fmt::println("material offset at: {:#x}", header.material_offset);
    f->write((void *)model_file_magic, strlen(model_file_magic));
    f->write(&header, sizeof(ModelHeader));

    u32 total_mesh_size = 0;
    for (auto &mesh : this->meshes) {
        MeshHeader mesh_header;
        mesh_header.vertex_size = mesh.vertices.size();
        mesh_header.index_size = mesh.indices.size();
        mesh_header.material_id = mesh.material_id;
        total_mesh_size += f->write(&mesh_header, sizeof(MeshHeader));
        total_mesh_size += f->write(mesh.vertices.data(),
                                    mesh.vertices.size() * sizeof(Vertex));
        total_mesh_size +=
            f->write(mesh.indices.data(), mesh.indices.size() * sizeof(u32));
    }
    fmt::println("write mesh data, size: {}", total_mesh_size);

    for (auto &texture : this->textures) {
        TextureField field;
        field.path_length = texture.length();
        f->write(&field, sizeof(TextureField));
        f->write((void *)texture.data(), field.path_length);
        fmt::println("write texture path: {}, length: {}", texture,
                     field.path_length);
    }

    for (auto &mat : this->materials) {
        MaterialField field;
        field.diffuse_map = mat.diffuse;
        field.specular_map = mat.specular;
        field.normal_map = mat.normal;
        f->write(&field, sizeof(MaterialField));
    }

    fmt::println("Succesfully dumped {}", file_path);
}