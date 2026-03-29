#ifndef _SEED_INSTANCE_DATA_H_
#define _SEED_INSTANCE_DATA_H_
#include "core/ref.h"
#include "core/transform.h"
#include "core/rendering/rhi/render_resource.h"
#include "rhi/render_resource.h"
#include <set>
#include <vector>
#include <list>

namespace Seed {

/* use buddy allocator to manage instance data */
class InstanceDataPool {
    public:
        struct Block {
                u32 idx;
                u32 size;
        };

    private:
        SSBOHandle ssbo_handle;
        u32 element_size;
        std::vector<std::list<Block>> free_zones;
        HandleOwner<Block> used_blocks;
        u32 max_order;
        Block split(Block &block);
        void merge(Block *b, u32 lg);

    public:
        u32 get_element_size(){
            return element_size;
        }
        Handle alloc(u32 size);
        void free(Handle handle);
        Block query(Handle handle);
        SSBOHandle get_render_buffer() { return ssbo_handle; }
        InstanceDataPool(u32 element_size, u32 size);
        ~InstanceDataPool();
};

class InstanceData : public RefCounted {
    protected:
        InstanceData(InstanceDataPool *pool) : pool(pool) {}
        InstanceDataPool *pool = nullptr;
        Handle instance_handle = NULL_HANDLE;
        void _upload(RHI::UpdateBufferInfo &update_info);

    public:
        virtual void upload() = 0;
        virtual u32 size() = 0;
        virtual void frustum_culling(const Frustum &frustum,
                                     const AABB &bounding_box,
                                     std::vector<u32> &instance_ids,
                                     std::vector<f32> &depths) = 0;

        virtual ~InstanceData();
};

class TransformInstanceData : public InstanceData {
    private:
        std::set<Ref<Transform>> transforms;

    public:
        const std::set<Ref<Transform>> &get_transforms() { return transforms; }
        u32 size() override { return transforms.size(); }
        void insert_transform(Ref<Transform> transform);
        void remove_transform(Ref<Transform> transform);
        void upload() override;
        void frustum_culling(const Frustum &frustum, const AABB &bounding_box,
                             std::vector<u32> &instance_ids,
                             std::vector<f32> &depths) override;

        TransformInstanceData();
};
}  // namespace Seed

#endif