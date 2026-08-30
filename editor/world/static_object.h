#ifndef _SEED_STATIC_OBJECT_H_
#define _SEED_STATIC_OBJECT_H_
#include "core/resource/model.h"
#include "core/transform.h"
namespace Seed {
struct ObjectInstance {
        alignas(4) u16 object_id;
        Mat4 world_mat;
};

class ObjectInstanceBatch : public InstanceBase<ObjectInstanceBatch> {
    private:
        SparseSet<ObjectInstance> instances;

    public:
        u32 size() override { return instances.size(); }
        /* Append one transform and defer the GPU copy until upload(). */
        Handle insert(u16 id, const Transform &transform);
        void update(Handle handle, const Transform &transform);
        void remove(Handle handle);
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