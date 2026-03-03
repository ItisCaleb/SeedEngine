#include "model_loader.h"

#include <spdlog/spdlog.h>
#include "core/io/file.h"
#include <filesystem>
#include "core/resource/model_file.h"
#include <algorithm>
#include <nfd.h>
#include "core/resource/resource_loader.h"
#include "core/resource/model.h"
#include "core/engine.h"
#include "core/serialize/json_impl.h"
#include <assimp/aabb.h>
#include <assimp/material.h>

using namespace Seed;

inline static Seed::Mat4 from_assimp(aiMatrix4x4 &mat) {
    Mat4 t;
    t[0][0] = mat.a1;
    t[0][1] = mat.a2;
    t[0][2] = mat.a3;
    t[0][3] = mat.a4;
    t[1][0] = mat.b1;
    t[1][1] = mat.b2;
    t[1][2] = mat.b3;
    t[1][3] = mat.b4;
    t[2][0] = mat.c1;
    t[2][1] = mat.c2;
    t[2][2] = mat.c3;
    t[2][3] = mat.c4;
    t[3][0] = mat.d1;
    t[3][1] = mat.d2;
    t[3][2] = mat.d3;
    t[3][3] = mat.d4;
    return t;
}

inline static Vec3 from_assimp(aiVector3D &v) { return Vec3{v.x, v.y, v.z}; }

inline static Vec2 from_assimp(aiVector2D &v) { return Vec2{v.x, v.y}; }

inline static Quaternion from_assimp(aiQuaternion &q) {
    return Quaternion{q.w, q.x, q.y, q.z};
}

i16 EditorModel::loadMaterialTextures(aiMaterial *mat, aiTextureType type) {
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

AABB EditorModel::calculateAABB(const std::vector<ModelVertex> &vertices) {
    f32 x1 = 1e5, x2 = -1e5;
    f32 y1 = 1e5, y2 = -1e5;
    f32 z1 = 1e5, z2 = -1e5;
    for (auto &vertex : vertices) {
        x1 = std::min(x1, vertex.position.x);
        x2 = std::max(x2, vertex.position.x);
        y1 = std::min(y1, vertex.position.y);
        y2 = std::max(y2, vertex.position.y);
        z1 = std::min(z1, vertex.position.z);
        z2 = std::max(z2, vertex.position.z);
    }
    f32 w = (x2 - x1) / 2;
    f32 h = (y2 - y1) / 2;
    f32 d = (z2 - z1) / 2;
    return AABB{Vec3{x2 - w, y2 - h, z2 - d}, Vec3{w, h, d}};
}

AABB EditorModel::calculateAABB(const std::vector<SkeletonVertex> &vertices) {
    f32 x1 = 1e5, x2 = -1e5;
    f32 y1 = 1e5, y2 = -1e5;
    f32 z1 = 1e5, z2 = -1e5;
    for (auto &vertex : vertices) {
        x1 = std::min(x1, vertex.position.x);
        x2 = std::max(x2, vertex.position.x);
        y1 = std::min(y1, vertex.position.y);
        y2 = std::max(y2, vertex.position.y);
        z1 = std::min(z1, vertex.position.z);
        z2 = std::max(z2, vertex.position.z);
    }
    f32 w = (x2 - x1) / 2;
    f32 h = (y2 - y1) / 2;
    f32 d = (z2 - z1) / 2;
    return AABB{Vec3{x2 - w, y2 - h, z2 - d}, Vec3{w, h, d}};
}

i32 EditorModel::get_bone_id(const std::string &name) {
    auto iter = bone_map.find(name);
    if (iter == bone_map.end()) {
        return -1;
    }
    return iter->second;
}

void EditorModel::processMesh(aiMesh *mesh, const aiScene *scene) {
    EditorMesh &m = meshes.emplace_back(EditorMesh{});
    std::vector<ModelVertex> &vertices = m.vertices;
    std::vector<u32> &indices = m.indices;
    for (int i = 0; i < mesh->mNumVertices; i++) {
        aiVector3D ai_vertex = mesh->mVertices[i];
        aiVector3D *ai_tex_coord = mesh->mTextureCoords[0];
        ModelVertex vertex;
        vertex.position = from_assimp(ai_vertex);
        if (mesh->mNormals) {
            vertex.normal = from_assimp(mesh->mNormals[i]);
            vertex.tangent = from_assimp(mesh->mTangents[i]);
        }
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
    m.material_id = get_material_id(mesh, scene);
}

void EditorModel::processBoneMesh(aiMesh *mesh, const aiScene *scene) {
    EditorBoneMesh &m = bone_meshes.emplace_back(EditorBoneMesh{});
    std::vector<SkeletonVertex> &vertices = m.vertices;
    std::vector<u32> &indices = m.indices;
    std::vector<u32> vertice_weights;
    for (int i = 0; i < mesh->mNumVertices; i++) {
        aiVector3D ai_vertex = mesh->mVertices[i];
        aiVector3D *ai_tex_coord = mesh->mTextureCoords[0];
        SkeletonVertex vertex;
        vertex.position = from_assimp(ai_vertex);
        if (mesh->mNormals) {
            vertex.normal = from_assimp(mesh->mNormals[i]);
            vertex.tangent = from_assimp(mesh->mTangents[i]);
        }
        if (ai_tex_coord) {
            vertex.tex_coord = {ai_tex_coord[i].x, ai_tex_coord[i].y};
        } else {
            vertex.tex_coord = {0, 0};
        }
        vertices.push_back(vertex);
    }
    vertice_weights.resize(vertices.size());
    if (mesh->mBones) {
        for (u32 i = 0; i < mesh->mNumBones; i++) {
            aiBone *bone = mesh->mBones[i];
            i32 bone_id = get_bone_id(bone->mName.C_Str());
            if (bone_id == -1) {
                bone_id = bones.size();
                this->bone_map[bone->mName.C_Str()] = bone_id;
                bones.push_back(
                    Bone{.name = bone->mName.C_Str(),
                         .offset_matrix = from_assimp(bone->mOffsetMatrix)});
            }

            for (u32 j = 0; j < bone->mNumWeights; j++) {
                u32 vertex_id = bone->mWeights[j].mVertexId;
                vertices[vertex_id].bone_ids[vertice_weights[vertex_id]] =
                    (u32)bone_id;
                vertices[vertex_id].bond_weights[vertice_weights[vertex_id]] =
                    bone->mWeights[j].mWeight;
                vertice_weights[vertex_id]++;
            }
        }
    }
    for (int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    m.material_id = get_material_id(mesh, scene);
}

i16 EditorModel::get_material_id(aiMesh *mesh, const aiScene *scene) {
    aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
    ::Material model_mat;
    model_mat.diffuse = loadMaterialTextures(mat, aiTextureType_DIFFUSE);
    model_mat.specular = loadMaterialTextures(mat, aiTextureType_SPECULAR);
    model_mat.normal = loadMaterialTextures(mat, aiTextureType_NORMALS);
    mat->Get(AI_MATKEY_OPACITY, &model_mat.opacity, nullptr);

    if (model_mat.normal == -1) {
        /* we try to load heightmap instead*/
        model_mat.normal = loadMaterialTextures(mat, aiTextureType_HEIGHT);
    }

    for (int i = 0; i < materials.size(); i++) {
        if (materials[i] == model_mat) {
            return i;
        }
    }
    if (!model_mat.is_null()) {
        materials.push_back(model_mat);
        return materials.size() - 1;
    }
    return -1;
}

void EditorModel::processNode(aiNode *node, const aiScene *scene) {
    for (int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        if (mesh->mBones) {
            processBoneMesh(mesh, scene);
        } else {
            processMesh(mesh, scene);
        }
    }
    for (int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

EditorModel::EditorModel(const std::string &path) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path, aiProcess_CalcTangentSpace | aiProcess_GenNormals |
                  aiProcess_Triangulate | aiProcess_OptimizeGraph |
                  aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices |
                  aiProcess_SortByPType);

    // If the import failed, report it
    if (!scene) {
        spdlog::error("Can't load Model from {}", path);
        return;
    }
    processNode(scene->mRootNode, scene);
    for (u32 i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation *_anim = scene->mAnimations[i];
        Animation &anim = this->animations.emplace_back(Animation{});
        anim.name = _anim->mName.C_Str();
        anim.duration = _anim->mDuration;
        anim.frames.reserve(_anim->mNumChannels);
        for (u32 j = 0; j < _anim->mNumChannels; j++) {
            aiNodeAnim *channel = _anim->mChannels[j];
            i32 bone_id = get_bone_id(channel->mNodeName.C_Str());
            anim.frames.push_back({});
            std::vector<Animation::KeyFrame> &frame = anim.frames.back();
            frame.reserve(channel->mNumPositionKeys);
            for (u32 k = 0; k < channel->mNumPositionKeys; k++) {
                frame.push_back(Animation::KeyFrame{
                    .position_time = channel->mPositionKeys[k].mTime,
                    .position = from_assimp(channel->mPositionKeys[k].mValue),
                    .rotation_time = channel->mRotationKeys[k].mTime,
                    .rotation = from_assimp(channel->mRotationKeys[k].mValue),
                    .scaling_time = channel->mScalingKeys[k].mTime,
                    .scaling = from_assimp(channel->mScalingKeys[k].mValue)});
            }
        }
    }
    fmt::println("{}", scene->mNumMeshes);
    std::filesystem::path dir = path;
    directory = dir.parent_path().filename().string();
}

template <typename json_type>
inline void to_json(json_type &j, const ::Material &m) {
    j = json_type{{"diffuse", m.diffuse},
                  {"specular", m.specular},
                  {"normal", m.normal},
                  {"opacity", m.opacity}};
}

void EditorModel::dump(const std::string &dir) {
    Ref<File> f = File::open(fmt::format("{}/{}.json", dir, directory), "wb");
    Ref<File> bin_f = File::open(fmt::format("{}/{}.bin", dir, directory), "wb");

    nlohmann::ordered_json j;
    j["name"] = dir;
    j["type"] = "model";
    j["textures"] = this->textures;
    j["materials"] = this->materials;
    j["meshes"] = nlohmann::json::array();
    j["bones"] = nlohmann::json::array();
    j["animations"] = nlohmann::json::array();
    j["bin_file"] = dir + ".bin";

    u64 offset = 0;
    for (auto &mesh : this->meshes) {
        nlohmann::ordered_json mesh_j;
        mesh_j["vertex_count"] = mesh.vertices.size();
        mesh_j["index_count"] = mesh.indices.size();
        mesh_j["material_id"] = mesh.material_id;
        mesh_j["bounding_box"] = calculateAABB(mesh.vertices);
        mesh_j["has_bone"] = false;
        mesh_j["bin_offset"] = offset;

        u64 bytes_size = mesh.vertices.size() * sizeof(ModelVertex) +
                         mesh.indices.size() * sizeof(u32);
        mesh_j["bin_size"] = bytes_size;
        bin_f->write(mesh.vertices.data(),
                     mesh.vertices.size() * sizeof(ModelVertex));
        bin_f->write(mesh.indices.data(), mesh.indices.size() * sizeof(u32));
        j["meshes"].push_back(mesh_j);
        offset += bytes_size;
    }

    for (auto &mesh : this->bone_meshes) {
        nlohmann::ordered_json mesh_j;
        mesh_j["vertex_count"] = mesh.vertices.size();
        mesh_j["index_count"] = mesh.indices.size();
        mesh_j["material_id"] = mesh.material_id;
        mesh_j["bounding_box"] = calculateAABB(mesh.vertices);
        mesh_j["has_bone"] = true;
        mesh_j["bin_offset"] = offset;

        u64 bytes_size = mesh.vertices.size() * sizeof(SkeletonVertex) +
                         mesh.indices.size() * sizeof(u32);
        mesh_j["bin_size"] = bytes_size;
        bin_f->write(mesh.vertices.data(),
                     mesh.vertices.size() * sizeof(SkeletonVertex));
        bin_f->write(mesh.indices.data(), mesh.indices.size() * sizeof(u32));
        j["meshes"].push_back(mesh_j);
        offset += bytes_size;
    }

    for (auto &bone : this->bones) {
        nlohmann::ordered_json bone_j;
        bone_j["name"] = bone.name;
        bone_j["bin_offset"] = offset;
        offset += bin_f->write(&bone.offset_matrix, sizeof(Mat4));
        j["bones"].push_back(bone_j);
    }

    for (auto &animation : this->animations) {
        nlohmann::ordered_json animation_j;
        animation_j["name"] = animation.name;
        animation_j["duration"] = animation.duration;
        animation_j["bin_offset"] = offset;
        for (auto &key_frame : animation.frames) {
            offset +=
                bin_f->write(key_frame.data(),
                             key_frame.size() * sizeof(Animation::KeyFrame));
            animation_j["key_frames"].push_back(key_frame.size());
        }
        j["animations"].push_back(animation_j);
    }
    f->write_str(j.dump(2));

    fmt::println("Succesfully dumped {}", dir);
}