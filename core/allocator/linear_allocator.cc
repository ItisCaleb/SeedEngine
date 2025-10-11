#include "linear_allocator.h"
#include <stdlib.h>
#include <string.h>
#include "core/math/utils.h"
#include <spdlog/spdlog.h>

namespace Seed {
void *LinearAllocator::alloc(u64 size) {
    std::lock_guard lg(mu);
    u64 new_size = this->cur + size;
    if (new_size > this->cap) {
        /* we need to store data when the capacity is not enough */
        void *buf = malloc(size);
        this->tmp_bufs.push_back(buf);
        this->overflow_size += (new_size) - this->cap;
        return buf;
    } else {
        this->cur = new_size;
        return (void *)((u64)this->memory_base + new_size - size);
    }
}

void *LinearAllocator::alloc_data(u64 size, void *data) {
    void *ptr = this->alloc(size);
    memcpy(ptr, data, size);
    return ptr;
}
void LinearAllocator::free_all() {
    /* free the tmp buffers and realloc */
    if (overflow_size > 0) {
        for (void *buf : tmp_bufs) {
            free(buf);
        }
        tmp_bufs.clear();
        this->cap = roundup_to_pow2(this->cap + this->overflow_size);
        this->memory_base = realloc(this->memory_base, this->cap);
        this->overflow_size = 0;
        spdlog::debug("Linear allocator resize to '{}'", this->cap);
    }
    this->cur = 0;
}
LinearAllocator::LinearAllocator() {
    u64 size = 1024 * 1024 * 16;
    this->memory_base = malloc(size);
    this->cap = size;
    this->cur = 0;
}
LinearAllocator::~LinearAllocator() { free(this->memory_base); }
}  // namespace Seed