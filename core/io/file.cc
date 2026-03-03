#include "file.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#ifdef _WIN32
#include <Windows.h>
#else 
#include <stdlib.h>
#endif

namespace Seed {
Ref<File> File::open(const std::string &path, const char *mode) {
    Ref<File> file;
    std::string fullpath = std::filesystem::absolute(path).string();
    FILE *f = fopen(fullpath.c_str(), mode);
    if (!f) {
        SPDLOG_WARN("Can't open file '{}'", fullpath);
        return file;
    }
    fseek(f, 0L, SEEK_END);
    u64 sz = ftell(f);
    fseek(f, 0L, SEEK_SET);
    file.create();
    file->file = f;
    file->path = path;
    file->full_path = fullpath;
    file->file_size = sz;
    file->read_cnt = 0;
    return file;
}
std::string File::read_str(size_t size) {
    std::string data;
    if (file && read_cnt < file_size) {
        if (size > file_size) {
            size = file_size;
        }
        read_cnt += size;
        data.resize(size);
        fread((void *)data.c_str(), 1, size, file);
    }
    return data;
}

void File::read(void *data, size_t size) {
    if (!data) {
        return;
    }
    if (file && read_cnt < file_size) {
        read_cnt += size;
        fread((void *)data, 1, size, file);
    }
}

nlohmann::json File::read_json() { return nlohmann::json::parse(file); }

size_t File::write(void *data, size_t size) {
    return fwrite(data, 1, size, file);
}

size_t File::write_str(const std::string &str) const {
    return fwrite(str.data(), 1, str.size(), file);
}

void File::copy_to(const std::string &path) const {
#ifdef _WIN32
    CopyFileA(this->full_path.c_str(), path.c_str(), false);
#else

#endif
}
}  // namespace Seed