#include "instance_data.h"
#include <cstring>
#include "core/collision/shape.h"
#include "core/macro.h"
#include "core/math/mat4.h"
#include "core/math/utils.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/debug/debug_drawer.h"
#include "core/engine.h"
#include "core/resource/model.h"
#include "core/transform.h"
#include "core/types.h"
#include "rhi/render_resource.h"

namespace Seed {

/* Since all instance is same size */
/* We use a slab allocator to implement the pool */
InstanceDataPool::Block InstanceDataPool::split(Block &block) {
    if (block.size == 1) return block;
    u32 halfsize = block.size >> 1;
    Block right = Block{.idx = block.idx + halfsize, .size = halfsize};
    this->free_zones[log2(halfsize)].push_back(right);
    Block left = Block{.idx = block.idx, .size = halfsize};
    return left;
}

Handle InstanceDataPool::alloc(u32 size) {
    /* get correspond zone */
    u32 lg = log2(roundup_to_pow2(size));
    if (lg >= this->max_order) return NULL_HANDLE;
    auto &zone = this->free_zones[lg];

    /* if there is one, then just give it. */
    if (!zone.empty()) {
        Handle h = this->used_blocks.insert(zone.front());
        zone.pop_front();
        return h;
    }

    /* or we split zone from upper recursively */
    for (u32 i = lg + 1; i < this->max_order; i++) {
        if (this->free_zones[i].empty()) continue;
        Block b = this->free_zones[i].front();
        this->free_zones[i].pop_front();
        while (1 << lg != b.size) {
            b = split(b);
        }

        Handle h = this->used_blocks.insert(b);
        return h;
    }
    return NULL_HANDLE;
}

void InstanceDataPool::merge(Block *b, u32 lg) {
    if (lg == this->max_order) return;
    Block block = *b;
    auto &zone = this->free_zones[lg];

    /* check the freed block is left or right, and get the pair block */
    u32 target_idx = ((block.idx >> lg) & 1) ? block.idx - block.size
                                             : block.idx + block.size;

    /* find the pair block in target zone  */
    for (auto it = zone.begin(); it != zone.end();) {
        Block buddy = *it;

        /* if the pair block is also free, the we merge recursively */
        if (buddy.idx == target_idx) {
            this->free_zones[lg + 1].push_back(
                Block{std::min(buddy.idx, block.idx), block.size << 1});
            merge(&this->free_zones[lg + 1].back(), lg + 1);
            it = zone.erase(it);
            return;
        } else {
            it++;
        }
    }
    /* no pair block found */
    this->free_zones[lg].push_back(block);
}
void InstanceDataPool::free(Handle handle) {
    Block *b = this->used_blocks.get_or_null(handle);
    EXPECT_NOT_NULL_RET(b);
    merge(b, log2(b->size));
    /* no merge */
    this->used_blocks.remove(handle);
}

InstanceDataPool::Block InstanceDataPool::query(Handle handle) {
    Block *b = this->used_blocks.get_or_null(handle);
    if (!b) return Block{0, 0};
    return *b;
}

InstanceDataPool::InstanceDataPool(u32 element_size, u32 size)
    : element_size(element_size) {
    this->max_order = log2(roundup_to_pow2(size)) + 1;
    this->ssbo_handle = RHI::alloc_storage_buffer(
        (1 << max_order) * element_size, UpdateFrequence::PERFRAME, nullptr);
    this->free_zones.resize(max_order);
    this->free_zones[this->max_order - 1].push_back(
        Block{0, 1u << (this->max_order - 1)});
}
InstanceDataPool::~InstanceDataPool() {}

void InstanceData::_upload(RHI::UpdateBufferInfo &update_info,
                           u32 element_offset) {
    u32 element_size = pool->get_element_size();
    u32 size = update_info.size / element_size;
    if (instance_handle == NULL_HANDLE) {
        instance_handle = pool->alloc(size);
    } else {
        auto block = pool->query(instance_handle);
        if (block.size < size) {
            if (block.size > 0) pool->free(instance_handle);
            instance_handle = pool->alloc(size);
        }
    }
    /* upload */
    InstanceDataPool::Block block = pool->query(instance_handle);
    u32 offset = element_size * block.idx + element_size * element_offset;
    RHI::update_from_heap(pool->get_render_buffer(), offset, update_info);
}

void InstanceData::_upload(std::vector<RHI::UpdateBufferInfo> &update_infos) {
    u32 element_size = pool->get_element_size();
    u32 total_size = 0;
    for (RHI::UpdateBufferInfo &info : update_infos) {
        total_size += info.size;
    }
    u32 size = total_size / element_size;
    if (instance_handle == NULL_HANDLE) {
        instance_handle = pool->alloc(size);
    } else {
        auto block = pool->query(instance_handle);
        if (block.size < size) {
            if (block.size > 0) pool->free(instance_handle);
            instance_handle = pool->alloc(size);
        }
    }
    /* upload */
    InstanceDataPool::Block block = pool->query(instance_handle);
    u32 offset = element_size * block.idx;
    for (RHI::UpdateBufferInfo &info : update_infos) {
        RHI::update_from_heap(pool->get_render_buffer(), offset, info);
        offset += info.size;
    }
}
InstanceData::~InstanceData() {
    if (this->instance_handle != NULL_HANDLE) {
        pool->free(this->instance_handle);
        this->instance_handle = NULL_HANDLE;
    }
}

void StaticInstanceData::insert_transform(Transform &transform) {
    this->world_matrices.push_back(transform.get_model_matrix());
    updated = false;
}
void StaticInstanceData::upload() {
    if (updated) {
        return;
    }
    RHI::UpdateBufferInfo mat_info =
        RHI::alloc_heap(sizeof(Mat4) * this->world_matrices.size());
    memcpy(mat_info.data, world_matrices.data(), mat_info.size);
    _upload(mat_info);
};
void StaticInstanceData::frustum_culling(const Frustum &frustum,
                                         const AABB &bounding_box,
                                         std::vector<u32> &instance_ids,
                                         std::vector<f32> &depths) {
    u32 i = pool->query(instance_handle).idx;
    for (Mat4 &mat : world_matrices) {
        AABB result = bounding_box.translate(mat);
        /* frustum culling */
        if (frustum.within_frustum(result)) {
            if (System::gEngine->get_debug_flag() &
                EngineConfig::BOUNDING_BOX) {
                System::gDebugDrawer->draw_aabb(result);
            }
            /* push instance indices */
            instance_ids.push_back(i);
            depths.push_back(frustum.calculate_depth(result.center));
        }
        i++;
    }
};

StaticInstanceData::StaticInstanceData()
    : InstanceData(
          System::gRenderEngine->get_instance_pool(TRANSFORM_POOL_NAME)) {}
}  // namespace Seed