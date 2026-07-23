#ifndef _SEED_INSTANCE_DATA_H_
#define _SEED_INSTANCE_DATA_H_
#include "core/collision/shape.h"
#include "core/math/mat4.h"
#include "core/ref.h"
#include "core/transform.h"
#include "core/rendering/rhi/render_resource.h"
#include <vector>
#include <list>

namespace Seed {

/*
 * Owns one shared GPU storage buffer and suballocates it in elements with a
 * buddy allocator. Block indices and sizes are element counts, not bytes.
 */
class InstanceDataPool {
    public:
        struct Block {
                /* First element in the shared storage buffer. */
                u32 idx;
                /* Allocated capacity in elements; always a power of two. */
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
        /* Allocate space for at least `size` elements. */
        Handle alloc(u32 size);
        void free(Handle handle);
        Block query(Handle handle);
        SSBOHandle get_render_buffer() { return ssbo_handle; }
        InstanceDataPool(u32 element_size, u32 size);
        ~InstanceDataPool();
};

/*
 * A CPU-side batch of logical instances backed by a block in an
 * InstanceDataPool.
 *
 * Typical lifecycle:
 *   1. Create the model-compatible batch with Model::create_instance().
 *   2. Register the same batch for every mesh in the model.
 *   3. clear() the previous logical instances when rebuilding the batch.
 *   4. Append each object through Model::add_instance() / _add_instance().
 *   5. The renderer calls upload(), then frustum_culling().
 *
 * Derived classes decide the CPU representation and how many pool elements
 * one logical instance occupies. The allocated GPU block remains owned by this
 * object across clear()/upload() calls and is released by the destructor.
 */
class InstanceData : public RefCounted {
    protected:
        InstanceData(InstanceDataPool *pool) : pool(pool) {}
        InstanceDataPool *pool = nullptr;
        Handle instance_handle = NULL_HANDLE;

        /*
         * Upload one contiguous payload to this batch's pool block. The block
         * is allocated on first upload and grows automatically when required.
         * `element_offset` is relative to the beginning of that block.
         */
        void _upload(RHI::UpdateBufferInfo &update_info,
                     u32 element_offset = 0);

        /*
         * Allocate for the combined payload and upload all buffers
         * consecutively into one pool block.
         */
        void _upload(std::vector<RHI::UpdateBufferInfo> &update_infos);

    public:
        /*
         * Upload the current CPU batch. This must happen before
         * frustum_culling(), because culling emits indices into the allocated
         * pool block.
         */
        virtual void upload() = 0;

        /* Number of logical instances currently stored in this batch. */
        virtual u32 size() = 0;

        /*
         * Number of pool elements occupied by one logical instance.
         * Static instances use one transform element. A skeletal instance uses
         * one transform followed by `bone_count` matrices.
         */
        virtual u32 instance_size() { return 1; }

        /*
         * Append visible absolute pool indices and matching depths to the
         * supplied vectors. This function does not clear either output.
         */
        virtual void frustum_culling(const Frustum &frustum,
                                     const AABB &bounding_box,
                                     std::vector<u32> &instance_ids,
                                     std::vector<f32> &depths) = 0;

        /*
         * Remove logical instances from the CPU batch. This intentionally
         * keeps the allocated pool block for reuse by a later upload().
         */
        virtual void clear() = 0;
        virtual ~InstanceData();
};

/* One logical instance is represented by one world transform matrix. */
class StaticInstanceData : public InstanceData {
    private:
        std::vector<Mat4> world_matrices;
        bool updated = false;

    public:
        u32 size() override { return world_matrices.size(); }
        /* Append one logical static-model instance; upload is deferred. */
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
