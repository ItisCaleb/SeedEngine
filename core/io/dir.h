#ifndef _SEED_DIR_H_
#define _SEED_DIR_H_
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "core/ref.h"

#ifndef SPLITOR
#ifdef _WIN32
#define SPLITOR "\\"
#else
#define SPLITOR "/"
#endif
#endif

namespace Seed {
class File;
class Dir : public RefCounted {
    private:
        std::string path;

    public:
        static Ref<Dir> open(const std::string &path);
        std::string concat(const std::string &path) {
            return this->path + SPLITOR + path;
        }
        static bool exists(const std::string &path) {
#ifdef _WIN32
            struct _stat s;
            _stat(path.c_str(), &s);
            return (s.st_mode & _S_IFDIR);
#else
            struct stat s;
            stat(path, &s);
            return S_ISDIR(s.st_mode);
#endif
        }
        Ref<File> open_file(const std::string &path, const char *mode = "rb");

        /* return false when fail to create */
        static bool create_if_not_exists(const std::string &path) {
            if (!exists(path)) {
                int r = _mkdir(path.c_str());
                /* 0 is succeess */
                return r == 0;
            }
            return true;
        }
};
}  // namespace Seed

#endif