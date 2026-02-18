#ifndef _SEED_HANDLE_H_
#define _SEED_HANDLE_H_
#include "core/types.h"
#include "core/container/freelist.h"
#include <mutex>

namespace Seed {

#define NULL_HANDLE (-1)
typedef i32 Handle;

template <typename T>
class HandleOwner {
    private:
        FreeList<T> datas;

    public:
        Handle insert(const T &d) {
            return datas.insert(d);
        }

        T *get_or_null(Handle h) {
            if (datas.present(h)) {
                return &datas[h];
            } else {
                return nullptr;
            }
        }

        void remove(Handle h) {
            datas.erase(h);
        }
};

template <typename T>
class HandleIdOwner {
    private:
        struct IDType{
            i32 id;
            T element;
        };
        FreeList<IDType> datas;
        i32 current_id = 0;

    public:
        Handle insert(const T &d) {
            i32 index = datas.insert(IDType{
                .id = current_id++,
                .element = d                
            });
            return index;
        }

        T *get_or_null(Handle h) {
            if (datas.present(h)) {
                return &datas[h].element;
            } else {
                return nullptr;
            }
        }

        i32 get_id(Handle h){
            if (datas.present(h)) {
                return datas[h].id;
            } else {
                return -1;
            }
        }

        void remove(Handle h) {
            datas.erase(h);
        }
};

template<typename T>
struct TypedHandle{
    Handle handle;
    constexpr TypedHandle() = default;
    constexpr TypedHandle(Handle handle):handle(handle) {}
    constexpr operator Handle() const { return handle;}
};

}  // namespace Seed

#endif