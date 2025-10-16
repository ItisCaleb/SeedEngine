#ifndef _SEED_INSTANCE_DATA_H_
#define _SEED_INSTANCE_DATA_H_
#include "core/ref.h"
#include "core/transform.h"
#include "core/rendering/api/render_resource.h"
#include "core/rendering/camera.h"
#include <set>
#include <vector>
#include <list>

namespace Seed {

class InstanceDataPool {
    public:
        struct Block {
                u32 idx;
                u32 size;
        };

    private:
        inline static InstanceDataPool *instance;

        RenderResource ssbo_rc;
        std::vector<std::list<Block>> free_zones;
        HandleOwner<Block> used_blocks;
        u32 max_order;
        Block split(Block &block);
        void merge(Block *b, u32 lg);

    public:
        Handle alloc(u32 size);
        void free(Handle handle);
        Block query(Handle handle);
        RenderResource get_render_buffer() { return ssbo_rc; }
        static InstanceDataPool *get_instance() { return instance; }
        InstanceDataPool();
        ~InstanceDataPool();
};

class InstanceData : public RefCounted {
    protected:
        InstanceData() {}
        Handle instance_handle = NULL_HANDLE;

    public:
        virtual void upload() = 0;
        virtual u32 get_size() = 0;
        virtual void frustum_culling(Camera *cam, const AABB &bounding_box,
                                     std::vector<u32> &instance_ids,
                                     std::vector<f32> &depths) = 0;

        virtual ~InstanceData();
};

class InstanceTransformData : public InstanceData {
    private:
        std::set<Ref<Transform>> transforms;

    public:
        const std::set<Ref<Transform>> &get_transforms() { return transforms; }
        u32 get_size() override { return transforms.size(); }
        void insert_transform(Ref<Transform> transform);
        void remove_transform(Ref<Transform> transform);
        void upload() override;
        void frustum_culling(Camera *cam, const AABB &bounding_box,
                             std::vector<u32> &instance_ids,
                             std::vector<f32> &depths) override;

        InstanceTransformData();
};
}  // namespace Seed

#endif