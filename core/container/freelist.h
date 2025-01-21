#ifndef _SEED_FREE_LIST_H_
#define _SEED_FREE_LIST_H_
#include <vector>
#include "core/types.h"

namespace Seed {
template <typename T>
class FreeList {
   public:
    /// Creates a new free list.
    FreeList();

    /// Inserts an element to the free list and returns an index to it.
    int insert(const T &element);

    // Removes the nth element from the free list.
    void erase(int n);

    // Removes all elements from the free list.
    void clear();

    // Returns the range of valid indices.
    int range() const;

    // Returns the nth element.
    T &operator[](int n);

    // Returns the nth element.
    const T &operator[](int n) const;

   private:
    union FreeElement {
        T element;
        int next;
    };
    std::vector<FreeElement> data;
    int first_free;
};


}  // namespace Seed

#endif