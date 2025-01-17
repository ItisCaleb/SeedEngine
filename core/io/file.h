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
    u64 size;
    u64 read_cnt;

   public:
    static Ref<File> open(const std::string &path, const char *mode) {
        FILE *f;
        fopen_s(&f, path.c_str(), mode);
        if (!f) {
            return Ref<File>();
        }
        fseek(f, 0L, SEEK_END);
        u64 sz = ftell(f);
        fseek(f, 0L, SEEK_SET);
        File *file = new File;
        file->file = f;
        file->size = sz;
        file->read_cnt = 0;
        return Ref<File>(file);
    }
    std::string read() {
        std::string data;
        if (file && read_cnt == 0) {
            data.resize(size);
            fread((void *)data.c_str(), 1, size, file);
        }
        return data;
    }

    ~File() {
        if (file) {
            fclose(file);
        }
    }
};

}  // namespace Seed
#endif