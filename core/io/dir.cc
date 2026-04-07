#include "dir.h"
#include "core/container/kstring.h"
#include "core/io/file.h"
#include <fmt/format.h>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace Seed {
Ref<Dir> Dir::open(KStr path) {
    Ref<Dir> dir;
    Path _path = path;
    if (!Dir::exists(_path)) {
        return dir;
    }
    dir.create();
    dir->path = std::move(_path);
    dir->path.absolute();
    return dir;
}

Ref<File> Dir::open_file(const KStr path, const char *mode) {
    return File::open(fmt::format("{}/{}", this->path.to_str(), path), mode);
}

std::vector<Path> Dir::list() {
    std::vector<Path> childrens;
#ifdef _WIN32
    // Windows 用 FindFirstFile / FindNextFile
    Path search = path;
    search.push("*");  // "foo/bar/*"

    WIN32_FIND_DATAA data;
    HANDLE h = FindFirstFileA(search.data(), &data);
    if (h == INVALID_HANDLE_VALUE) return childrens;

    do {
        KStr name(data.cFileName);
        if (name == "." || name == "..") continue;
        bool is_directory = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        childrens.push_back(name);
    } while (FindNextFileA(h, &data));

    FindClose(h);

#else
    // Linux/macOS 用 opendir / readdir
    DIR *dir = opendir(path.data());
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        KStr name(entry->d_name);
        if (name == "." || name == "..") continue;
        bool is_directory = entry->d_type == DT_DIR;
        childrens.push_back(name);
    }

    closedir(dir);
#endif
    return childrens;
}
}  // namespace Seed