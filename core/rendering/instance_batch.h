#ifndef _SEED_INSTANCE_BATCH_H_
#define _SEED_INSTANCE_BATCH_H_
#include "core/collision/shape.h"
#include "core/math/mat4.h"
#include "core/ref.h"
#include "core/transform.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/misc/type_name.h"
#include <vector>
#include <list>

namespace Seed {

/*
 * Owns one shared GPU storage buffer and suballocates it in elements with a
 * buddy allocator. Block indices and sizes are element counts, not bytes.
 */
class InstanceBatchPool {
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
        InstanceBatchPool(u32 element_size, u32 size);
        ~InstanceBatchPool();
};

/*
 * Stores the CPU representation of a group of logical instances.
 *
 * InstanceBatch owns the common upload and culling flow. Derived classes only
 * provide the data to upload, the number of logical instances, and the world
 * bounding box of each instance.
 *
 * Typical lifecycle:
 *   1. Create the model-compatible batch with Model::create_instance().
 *   2. Append logical instances through the model or batch-specific API.
 *   3. Mutating operations call mark_dirty().
 *   4. The renderer calls upload() before frustum_culling().
 *   5. clear() keeps the existing pool allocation available for reuse.
 *
 * instance_handle identifies this batch's allocation in its registered pool.
 * Its block indices and offsets are measured in pool elements, not bytes.
 */
class InstanceBatch : public RefCounted {
    protected:
        InstanceBatch() = default;
        Handle instance_handle = NULL_HANDLE;
        InstanceBatchPool *pool = nullptr;
        bool dirty = false;

    protected:
        /*
         * Mark the CPU representation as newer than the GPU copy. Every
         * operation that changes uploaded data must call this.
         */
        void mark_dirty() { dirty = true; }

    public:
        /*
         * Runtime key used by RenderEngine to identify this batch type.
         * InstanceBase<T> supplies it automatically for derived classes.
         */
        virtual u64 type_id() = 0;

        /*
         * Upload the current CPU representation when dirty. The pool block is
         * allocated on first upload and grows when the prepared data no longer
         * fits. A successful upload clears the dirty flag.
         */
        void upload();

        /*
         * Append visible absolute pool element indices and matching depths.
         * The supplied output vectors are appended to and are not cleared.
         * upload() must have established a valid pool allocation first.
         */
        void frustum_culling(const Frustum &frustum, const AABB &bounding_box,
                             std::vector<u32> &instance_ids,
                             std::vector<f32> &depths);

        /*
         * Build the contiguous upload regions for the current CPU data.
         * InstanceBatch uploads the returned regions consecutively into one
         * pool block.
         */
        virtual void prepare_uploads(
            std::vector<RHI::UpdateBufferInfo> &uploads) = 0;

        /*
         * Transform a mesh-local bounding box for logical instance `i`.
         * `i` is an index into the derived class's CPU instance container.
         */
        virtual AABB translate_bounding_box(const AABB &bounding_box,
                                            u32 i) = 0;

        /* Number of logical instances currently stored in this batch. */
        virtual u32 size() = 0;

        /*
         * Number of consecutive pool elements occupied by one logical
         * instance. This advances the absolute GPU element index during
         * culling. Static and terrain instances occupy one element; a skeletal
         * instance occupies one transform plus its bone matrices.
         */
        virtual u32 element_per_instance() { return 1; }

        /*
         * Remove all logical CPU instances without discarding the reusable pool
         * allocation. Implementations must mark the batch dirty when the empty
         * state needs to be reflected on the GPU.
         */
        virtual void clear() = 0;
        virtual ~InstanceBatch();
};

/*
 * Supplies a stable engine type ID and resolves the pool registered for T.
 * Derived batches inherit from InstanceBase<Derived> instead of repeating the
 * type_id() implementation and pool lookup.
 */
template <typename T>
class InstanceBase : public InstanceBatch {
    protected:
        InstanceBase() {
            pool = System::gRenderEngine->get_instance_pool(Seed::type_id<T>());
        }

    public:
        u64 type_id() override final { return Seed::type_id<T>(); }
};

/*
 * Stores one world transform matrix for each logical static-model instance.
 */
class StaticInstanceBatch : public InstanceBase<StaticInstanceBatch> {
    private:
        std::vector<Mat4> world_matrices;

    public:
        u32 size() override { return world_matrices.size(); }
        /* Append one transform and defer the GPU copy until upload(). */
        void insert_transform(Transform &transform);
        void prepare_uploads(
            std::vector<RHI::UpdateBufferInfo> &uploads) override;
        AABB translate_bounding_box(const AABB &bounding_box, u32 i) override;

        void clear() override { this->world_matrices.clear(); }

        StaticInstanceBatch();
};

}  // namespace Seed

#endif
