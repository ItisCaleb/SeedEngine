#ifndef _SEED_HANDLE_H_
#define _SEED_HANDLE_H_
#include "core/types.h"
#include "core/container/freelist.h"

namespace Seed {
typedef i32 Handle;

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

        void get_used(std::vector<T*> &v){
            for(i32 i = 0;i<datas.range();i++){
                if(datas.present(i)){
                    v.push_back(&datas[i]);
                }
            }
        }

        void remove(Handle h) { datas.erase(h); }
};

}  // namespace Seed

#endif