#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_
#include "core/ref.h"
#include "api/render_resource.h"
#include "mesh.h"
#include <vector>

namespace Seed {

class RenderEngine;
class ModelMaterial {
   private:
    std::vector<std::vector<Ref<Material>>> materials;
    Ref<Material> default_mat;
   public:
    ModelMaterial(const std::vector<Ref<Material>> &mat);
    ~ModelMaterial() = default;
    std::vector<Ref<Material>> *get_materials(u32 variant = 0);
    Ref<Material> get_material(u32 material_handle, u32 variant = 0);
    void add_variant(std::vector<Ref<Material>> &variant);
};

class Model : public RefCounted {
    friend RenderEngine;

   private:
    RenderResource vertices_desc_rc;
    RenderResource instance_rc;
    RenderResource instance_desc_rc;

    std::vector<Mesh> meshes;
    ModelMaterial model_mat;

   public:
    Model(const std::vector<Mesh> &meshes,const  std::vector<Ref<Material>> &mats);
    ~Model();
    static Ref<Model> create(const std::vector<Mesh> &meshes,
                             const std::vector<Ref<Material>> &mats);
};
}  // namespace Seed

#endif