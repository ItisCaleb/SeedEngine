#ifndef _SEED_FREE_LIST_H_
#define _SEED_FREE_LIST_H_
#include <vector>
#include "core/types.h"

namespace Seed {
template <typename T>
class FreeList {
    public:
        /// Creates a new free list.
        FreeList() : first_free(-1) {}

        /// Inserts an element to the free list and returns an index to it.
        int insert(const T &element) {
            if (first_free != -1) {
                const int index = first_free;
                first_free = data[index].next;
                data[index].element = element;
                data[index].next = -1;
                return index;
            } else {
                FreeElement fe;
                fe.element = element;
                fe.next = -1;
                data.push_back(fe);
                return static_cast<int>(data.size() - 1);
            }
        }

        // Removes the nth element from the free list.
        void erase(int n) {
            data[n].next = first_free;
            first_free = n;
        }

        // Removes all elements from the free list.
        void clear() {
            data.clear();
            first_free = -1;
        }

        // Returns the range of valid indices.
        int range() const { return static_cast<int>(data.size()); }

        // Returns the nth element.
        T &operator[](int n) {
            if (data[n].next != -1) {
                throw std::runtime_error("Accessing invalid freelist element");
            }
            return data[n].element;
        }

        // Returns the nth element.
        const T &operator[](int n) const {
            if (data[n].next != -1) {
                throw std::runtime_error("Accessing invalid freelist element");
            }
            return data[n].element;
        }

        bool present(int n) const{
            return data[n].next == -1;
        }

    private:
        struct FreeElement {
                T element;
                int next;
        };
        std::vector<FreeElement> data;
        int first_free;
};

}  // namespace Seed

#endif