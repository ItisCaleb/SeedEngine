#include "dir.h"
#include "core/io/file.h"
#include <fmt/format.h>

namespace Seed {
Ref<Dir> Dir::open(const std::string &path) {
    Ref<Dir> dir;
    if(!Dir::exists(path)){
        return dir;
    }
    dir.create();
    dir->path = std::filesystem::absolute(path).string();
    return dir;
}

Ref<File> Dir::open_file(const std::string &path, const char *mode) {
    return File::open(fmt::format("{}/{}", this->path, path), mode);
}
}  // namespace Seed