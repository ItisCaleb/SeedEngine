#ifndef _SEED_FILE_H_
#define _SEED_FILE_H_
#include "core/ref.h"
#include "core/types.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <fmt/format.h>
namespace Seed {
class File : public RefCounted {
   private:
    FILE *file;
    u64 file_size;
    u64 read_cnt;
    u64 write_cnt;

   public:
    static Ref<File> open(const std::string &path, const char *mode) {
        FILE *f = fopen(path.c_str(), mode);
        if (!f) {
            return Ref<File>();
        }
        fseek(f, 0L, SEEK_END);
        u64 sz = ftell(f);
        fseek(f, 0L, SEEK_SET);
        File *file = new File;
        file->file = f;
        file->file_size = sz;
        file->read_cnt = 0;
        return Ref<File>(file);
    }
    std::string read_str(size_t size=0) {
        std::string data;
        if (file && read_cnt < file_size) {
            if(size == 0 || size > file_size){
                size = file_size;
            }
            read_cnt += size;
            data.resize(size);
            fread((void *)data.c_str(), 1, size, file);
        }
        return data;
    }

    template<typename T>
    void read(T *data) {
        if(data == nullptr) return;
        size_t size = sizeof(T);

        if (file && read_cnt < file_size) {
            read_cnt += size;
            fread((void *)data, 1, size, file);
        }
    }

    template<typename T>
    void read_vector(std::vector<T> &vec, u32 cnt) {
        size_t ele_size = sizeof(T);
        vec.resize(cnt);
        if (file && read_cnt < file_size) {
            read_cnt += cnt * ele_size;

            fread((void *)vec.data(), 1, cnt * ele_size, file);
        }
    }

    size_t write(void *data, size_t size){
        return fwrite(data, 1, size, file);
    }

    ~File() {
        if (file) {
            fclose(file);
        }
    }
};

}  // namespace Seed
#endif