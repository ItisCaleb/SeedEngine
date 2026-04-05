#include "model_loader.h"

#include <spdlog/spdlog.h>
#include "core/container/kstring.h"
#include "core/io/file.h"
#include <filesystem>
#include "core/io/path.h"
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

i16 EditorModel::get_bone_id(const std::string &name) {
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
        for (u32 j = 0; j < 4; j++) {
            vertex.bone_ids[j] = 0;
            vertex.bone_weights[j] = 0.0f;
        }
        vertices.push_back(vertex);
    }
    vertice_weights.resize(vertices.size());
    if (mesh->mBones) {
        for (u32 i = 0; i < mesh->mNumBones; i++) {
            aiBone *bone = mesh->mBones[i];
            i16 bone_id = get_bone_id(bone->mName.C_Str());
            this->bones[bone_id].offset_matrix =
                from_assimp(bone->mOffsetMatrix);

            for (u32 j = 0; j < bone->mNumWeights; j++) {
                u32 vertex_id = bone->mWeights[j].mVertexId;
                u32 weight_count = vertice_weights[vertex_id];
                if (weight_count >= 4) {
                    continue;
                }
                vertices[vertex_id].bone_ids[weight_count] = (u16)bone_id;
                vertices[vertex_id].bone_weights[weight_count] =
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

void EditorModel::processAnimation(const aiScene *scene) {
    for (u32 i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation *_anim = scene->mAnimations[i];
        EditorAnimation &anim =
            this->animations.emplace_back(EditorAnimation{});
        anim.name = _anim->mName.C_Str();
        anim.duration = _anim->mDuration;
        anim.clips.reserve(_anim->mNumChannels);
        for (u32 j = 0; j < _anim->mNumChannels; j++) {
            aiNodeAnim *channel = _anim->mChannels[j];
            i16 bone_id = get_bone_id(channel->mNodeName.C_Str());
            anim.clips.push_back(AnimationClip{.bone_id = (u16)bone_id});
            AnimationClip &clip = anim.clips.back();
            clip.position_keys.reserve(channel->mNumPositionKeys);
            clip.rotation_keys.reserve(channel->mNumRotationKeys);
            clip.scaling_keys.reserve(channel->mNumScalingKeys);
            for (u32 k = 0; k < channel->mNumPositionKeys; k++) {
                clip.position_keys.push_back(PositionKey{
                    .position_time = channel->mPositionKeys[k].mTime,
                    .position = from_assimp(channel->mPositionKeys[k].mValue)});
            }
            for (u32 k = 0; k < channel->mNumRotationKeys; k++) {
                clip.rotation_keys.push_back(RotationKey{
                    .rotation_time = channel->mRotationKeys[k].mTime,
                    .rotation = from_assimp(channel->mRotationKeys[k].mValue)});
            }
            for (u32 k = 0; k < channel->mNumScalingKeys; k++) {
                clip.scaling_keys.push_back(ScalingKey{
                    .scaling_time = channel->mScalingKeys[k].mTime,
                    .scaling = from_assimp(channel->mScalingKeys[k].mValue)});
            }
        }
    }
}

void EditorModel::processBoneHierachy(aiNode *node, const aiScene *scene,
                                      u16 parent_id) {
    std::string name = node->mName.C_Str();
    bool is_bone = this->bone_names.count(name) > 0;
    if (is_bone) {
        Bone bone = Bone{
            .name = name,
            .parent = (u16)parent_id,
        };
        parent_id = this->bones.size();
        this->bone_map[name] = parent_id;
        this->bones.push_back(bone);
    }
    for (int i = 0; i < node->mNumChildren; i++) {
        processBoneHierachy(node->mChildren[i], scene, parent_id);
    }
}

void EditorModel::collectBoneNames(aiNode *node, const aiScene *scene) {
    for (int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        if (mesh->mBones) {
            for (u32 i = 0; i < mesh->mNumBones; i++) {
                aiBone *bone = mesh->mBones[i];
                this->bone_names.insert(bone->mName.C_Str());
            }
        }
    }
    for (int i = 0; i < node->mNumChildren; i++) {
        collectBoneNames(node->mChildren[i], scene);
    }
}

EditorModel::EditorModel(const std::string &path) {
    Assimp::Importer importer;
    this->origin_path = path;
    u32 assimp_flag = aiProcess_CalcTangentSpace | aiProcess_GenNormals |
                      aiProcess_Triangulate | aiProcess_OptimizeGraph |
                      aiProcess_OptimizeMeshes |
                      aiProcess_JoinIdenticalVertices | aiProcess_SortByPType;
    std::string_view view(path);
    view = view.substr(view.find_last_of(".") + 1);
    std::string extension = std::string(view);
    if (extension == "gltf") {
        assimp_flag |= aiProcess_FlipUVs;
    }

    const aiScene *scene = importer.ReadFile(path, assimp_flag);

    // If the import failed, report it
    if (!scene) {
        spdlog::error("Can't load Model from {}", path);
        return;
    }
    collectBoneNames(scene->mRootNode, scene);
    processBoneHierachy(scene->mRootNode, scene, 0);
    processNode(scene->mRootNode, scene);
    processAnimation(scene);

    fmt::println("{}", scene->mNumMeshes);
    std::filesystem::path dir = path;
    origin_dir = dir.parent_path();
}

template <typename json_type>
inline void to_json(json_type &j, const ::Material &m) {
    j = json_type{{"diffuse", m.diffuse},
                  {"specular", m.specular},
                  {"normal", m.normal},
                  {"opacity", m.opacity}};
}

void EditorModel::dump(const Seed::Path &dir) {
    std::string name = origin_dir.filename().string();
    Ref<File> f = File::open(fmt::format("{}/{}.json", dir, name), "wb");
    Ref<File> bin_f = File::open(fmt::format("{}/{}.bin", dir, name), "wb");
    for (KStr texture : this->textures) {
        Ref<File> t =
            File::open(fmt::format("{}/{}", origin_dir.string(), texture));
        t->copy_to(fmt::format("{}/{}", dir, t->get_filename()));
    }

    nlohmann::ordered_json j;
    j["name"] = name;
    j["type"] = "model";
    j["textures"] = this->textures;
    j["materials"] = this->materials;
    j["bin_file"] = name + ".bin";
    j["meshes"] = nlohmann::json::array();
    j["bones"] = {};
    j["animations"] = nlohmann::json::array();

    u64 offset = 0;
    for (auto &mesh : this->meshes) {
        nlohmann::ordered_json mesh_j;
        mesh_j["vertex_count"] = mesh.vertices.size();
        mesh_j["index_count"] = mesh.indices.size();
        mesh_j["material_id"] = mesh.material_id;
        mesh_j["bounding_box"] = calculateAABB(mesh.vertices);
        mesh_j["has_bone"] = false;
        mesh_j["bin_offset"] = offset;

        u64 _offset = offset;
        offset += bin_f->write(mesh.vertices);
        offset += bin_f->write(mesh.indices);
        mesh_j["bin_size"] = offset - _offset;
        j["meshes"].push_back(mesh_j);
    }

    for (auto &mesh : this->bone_meshes) {
        nlohmann::ordered_json mesh_j;
        mesh_j["vertex_count"] = mesh.vertices.size();
        mesh_j["index_count"] = mesh.indices.size();
        mesh_j["material_id"] = mesh.material_id;
        mesh_j["bounding_box"] = calculateAABB(mesh.vertices);
        mesh_j["has_bone"] = true;
        mesh_j["bin_offset"] = offset;
        u64 _offset = offset;
        offset += bin_f->write(mesh.vertices);
        offset += bin_f->write(mesh.indices);
        mesh_j["bin_size"] = offset - _offset;
        j["meshes"].push_back(mesh_j);
    }

    if (!this->bones.empty()) {
        j["bones"]["bin_offset"] = offset;
        j["bones"]["bin_size"] = this->bones.size() * sizeof(Mat4);
        j["bones"]["names"] = nlohmann::json::array();
        j["bones"]["parents"] = nlohmann::json::array();
        for (auto &bone : this->bones) {
            j["bones"]["names"].push_back(bone.name);
            j["bones"]["parents"].push_back(bone.parent);
            offset += bin_f->write(&bone.offset_matrix);
        }
    }

    for (auto &animation : this->animations) {
        nlohmann::ordered_json animation_j;
        animation_j["name"] = animation.name;
        animation_j["duration"] = animation.duration;
        animation_j["clip_count"] = animation.clips.size();
        animation_j["bin_offset"] = offset;
        u64 _offset = offset;
        for (auto &clip : animation.clips) {
            struct ClipInfo {
                    u16 bone_id;
                    u16 position_key_count;
                    u16 rotation_key_count;
                    u16 scaling_key_count;
            } clip_info;
            clip_info.bone_id = clip.bone_id;
            clip_info.position_key_count = clip.position_keys.size();
            clip_info.rotation_key_count = clip.rotation_keys.size();
            clip_info.scaling_key_count = clip.scaling_keys.size();
            offset += bin_f->write(&clip_info);
            offset += bin_f->write(clip.position_keys);
            offset += bin_f->write(clip.rotation_keys);
            offset += bin_f->write(clip.scaling_keys);
        }
        animation_j["bin_size"] = offset - _offset;
        j["animations"].push_back(animation_j);
    }
    f->write_str(j.dump(2));

    fmt::println("Succesfully dumped {}", dir);
}