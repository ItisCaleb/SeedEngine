#ifndef _SEED_MODEL_FILE_H_
#define _SEED_MODEL_FILE_H_
#include "core/types.h"
#include "core/collision/aabb.h"
inline const static char *model_file_magic = "SEEDMDL0";

#pragma pack(push, 1)
struct ModelHeader {
    u32 mesh_count;
    u16 texture_count;
    u16 material_count;
    /* offset in model file*/
    u32 mesh_offset;
    u32 texture_offset;
    u32 material_offset;
    Seed::AABB bounding_box;
};

struct MeshHeader {
    u32 vertex_size;
    u32 index_size;
    i16 material_id;
};

struct TextureField {
    u16 path_length;
    char path[];
};

struct MaterialField {
    /* texture ids*/
    u16 diffuse_map;
    u16 specular_map;
    u16 normal_map;
};
#pragma pack(pop)



    /*
    |-----------------|
    |    "SEEDMDL"    |
    |-----------------|
    |   model header  |
    |-----------------|
    |   mesh header   |
    |-----------------|
    |    vertices     |
    |-----------------|
    |     indices     |
    |-----------------|
    |  texture fields |
    |-----------------|
    | material fields |
    |-----------------|

    */

#endif