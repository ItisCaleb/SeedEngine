#ifndef _SEED_INSTANCE_DATA_H_
#define _SEED_INSTANCE_DATA_H_
#include "core/collision/shape.h"
#include "core/math/mat4.h"
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
        u32 get_element_size() { return element_size; }
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
        void _upload(RHI::UpdateBufferInfo &update_info,
                     u32 element_offset = 0);
        void _upload(std::vector<RHI::UpdateBufferInfo> &update_infos);

    public:
        virtual void upload() = 0;
        virtual u32 size() = 0;
        /* size for single instance */
        /* if instance only contains transform, then instance size is 1 */
        /* if instance contains transform and skeleton, */
        /* then instance size is 1 + bone_count */
        virtual u32 instance_size() {
            return 1;;
        }
        virtual void frustum_culling(const Frustum &frustum,
                                     const AABB &bounding_box,
                                     std::vector<u32> &instance_ids,
                                     std::vector<f32> &depths) = 0;
        virtual void clear() = 0;
        virtual ~InstanceData();
};

class StaticInstanceData : public InstanceData {
    private:
        std::vector<Mat4> world_matrices;
        bool updated = false;

    public:
        u32 size() override { return world_matrices.size(); }
        void insert_transform(Transform &transform);
        void upload() override;
        void frustum_culling(const Frustum &frustum, const AABB &bounding_box,
                             std::vector<u32> &instance_ids,
                             std::vector<f32> &depths) override;
        void clear() override { this->world_matrices.clear(); }

        StaticInstanceData();
};

}  // namespace Seed

#endif