#include "instance_data.h"
#include "core/macro.h"
#include "core/math/utils.h"
#include "core/rendering/api/render_command.h"

namespace Seed {

InstanceDataPool::Block InstanceDataPool::split(Block &block) {
    if (block.size == 1) return block;
    u32 halfsize = block.size >> 1;
    Block right = Block{.idx = block.idx + halfsize, .size = halfsize};
    this->free_zones[log2(halfsize)].push_back(right);
    Block left = Block{.idx = block.idx, .size = halfsize};
    return left;
}

Handle InstanceDataPool::alloc(u32 size) {
    u32 lg = log2(roundup_to_pow2(size));
    if (lg >= this->max_order) return NULL_HANDLE;
    auto &zone = this->free_zones[lg];
    if (!zone.empty()) {
        Handle h = this->used_blocks.insert(zone.front());
        zone.pop_front();
        return h;
    } else {
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
}

void InstanceDataPool::merge(Block *b, u32 lg) {
    if (lg == this->max_order) return;
    Block block = *b;
    auto &zone = this->free_zones[lg];
    bool target_idx = ((block.idx >> lg) & 1) ? block.idx - block.size
                                              : block.idx + block.size;

    for (auto it = zone.begin(); it != zone.end();) {
        Block buddy = *it;
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

InstanceDataPool::InstanceDataPool() {
    instance = this;
    this->max_order = 20;
    this->ssbo_rc.alloc_buffer((1 << max_order) * sizeof(Mat4), nullptr);
    this->free_zones.resize(max_order);
    this->free_zones[this->max_order - 1].push_back(
        Block{0, 1u << (this->max_order - 1)});
}
InstanceDataPool::~InstanceDataPool() {}
void InstanceData::insert_transform(Ref<Transform> transform) {
    EXPECT_NOT_NULL_RET(*transform);
    this->transforms.insert(transform);
}

void InstanceData::remove_transform(Ref<Transform> transform) {
    EXPECT_NOT_NULL_RET(*transform);
    this->transforms.erase(transform);
}

void InstanceData::upload() {
    InstanceDataPool *pool = InstanceDataPool::get_instance();
    if (instance_handle == NULL_HANDLE) {
        instance_handle = pool->alloc(this->transforms.size());
    } else {
        auto block = pool->query(instance_handle);
        if (block.size < this->transforms.size()) {
            pool->free(instance_handle);
            instance_handle = pool->alloc(this->transforms.size());
        }
    }
    /* upload */
    InstanceDataPool::Block block = pool->query(instance_handle);
    RenderCommandDispatcher dp;
    Mat4 *mats = (Mat4 *)dp.map_buffer(pool->get_render_buffer(),
                                       sizeof(Mat4) * block.idx,
                                       sizeof(Mat4) * this->transforms.size());
    u32 i = 0;
    for (Ref<Transform> transform : this->transforms) {
        mats[i] = transform->get_model_matrix().transpose();
        i++;
    }
}

u32 InstanceData::get_start_idx() {
    InstanceDataPool *pool = InstanceDataPool::get_instance();
    return pool->query(instance_handle).idx;
}

InstanceData::InstanceData() {}
InstanceData::~InstanceData() {}
}  // namespace Seed