#ifndef _SEED_HANDLE_H_
#define _SEED_HANDLE_H_
#include "core/types.h"
#include "core/container/freelist.h"

namespace Seed {
typedef u32 Handle;

template <typename T>
class HandleOwner {
    private:
        FreeList<T> datas;

    public:
        Handle insert(const T &d) { return datas.insert(d); }

        T *get_or_null(Handle h) {
            if (datas.present(h)) {
                return &datas[h];
            } else {
                return nullptr;
            }
        }

        void remove(Handle h) { datas.erase(h); }
};

}  // namespace Seed

#endif