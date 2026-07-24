#ifndef _SEED_STATIC_OBJECT_H_
#define _SEED_STATIC_OBJECT_H_
#include "core/resource/model.h"
namespace Seed {
struct ObjectInstance {
        alignas(4) u16 object_id;
        Mat4 world_mat;
};

class ObjectInstanceBatch : public InstanceBase<ObjectInstanceBatch> {
    private:
        std::vector<ObjectInstance> instances;

    public:
        u32 size() override { return instances.size(); }
        /* Append one transform and defer the GPU copy until upload(). */
        void insert_object(u16 id, Transform &transform);
        void prepare_uploads(
            std::vector<RHI::UpdateBufferInfo> &uploads) override;
        AABB translate_bounding_box(const AABB &bounding_box, u32 i) override;

        void clear() override { this->instances.clear(); }
};

struct StaticObject {
        u16 id;
        Transform transform;
        Ref<BasicModel> model;
        Ref<ObjectInstanceBatch> instances;
};

}  // namespace Seed

#endif