#ifndef _SEED_FREE_LIST_H_
#define _SEED_FREE_LIST_H_
#include "core/types.h"
#include <mutex>

namespace Seed {
template <typename T, u32 chunk_size = 256>
class FreeList {
    private:
        struct FreeElement {
                T element;
                int next;
        };
        constexpr static const u32 MAX_CHUNK = 4;
        FreeElement *chunks[MAX_CHUNK] = {};
        i32 first_free = -1;
        std::atomic<i32> cap;
        std::mutex chunk_lock;
        inline FreeElement *get_element(i32 index) const {
            return &chunks[index / chunk_size][index % chunk_size];
        }

    public:
        /// Creates a new free list.
        FreeList() { chunks[0] = new FreeElement[chunk_size]; }

        /// Inserts an element to the free list and returns an index to it.
        i32 insert(const T &element) {
            if (first_free != -1) {
                const i32 index = first_free;
                FreeElement *data = get_element(index);
                first_free = data->next;
                data->element = element;
                data->next = -1;
                return index;
            } else {
                i32 index = this->cap++;
                u32 chunk = index / chunk_size;
                /* prevent multiple threads alloc chunk */
                if (this->chunks[chunk] == nullptr) {
                    chunk_lock.lock();
                    if (this->chunks[chunk] == nullptr) {
                        chunks[chunk] = new FreeElement[chunk_size];
                    }
                    chunk_lock.unlock();
                }
                chunks[chunk][index % chunk_size] =
                    FreeElement{.element = element, .next = -1};
                return index;
            }
        }

        bool present(int n) const {
            if (n < 0 || n >= this->cap) return false;
            return get_element(n)->next == -1;
        }

        // Removes the nth element from the free list.
        void erase(int n) {
            if (!present(n)) return;
            FreeElement *data = get_element(n);
            data->element.~T();
            data->next = first_free;
            first_free = n;
            this->cap--;
        }

        void clear(){
            first_free = -1;
            this->cap = 0;
        }

        // Returns the usage count
        int count() const { return this->cap; }

        // Returns the nth element.
        T &operator[](int n) {
            FreeElement *data = get_element(n);
            if (data->next != -1) {
                throw std::runtime_error("Accessing invalid freelist element");
            }
            return data->element;
        }

        // Returns the nth element.
        const T &operator[](int n) const {
            FreeElement *data = get_element(n);
            if (data->next != -1) {
                throw std::runtime_error("Accessing invalid freelist element");
            }
            return data->element;
        }
};

}  // namespace Seed

#endif