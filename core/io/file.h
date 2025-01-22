#ifndef _SEED_FILE_H_
#define _SEED_FILE_H_
#include "core/ref.h"
#include "core/types.h"
#include <stdio.h>

#include <string>
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
    std::string read(size_t size=0) {
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

    void write(void *data, size_t size){
        fwrite(data, 1, size, file);
    }

    ~File() {
        if (file) {
            fclose(file);
        }
    }
};

}  // namespace Seed
#endif