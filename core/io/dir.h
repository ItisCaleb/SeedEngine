#ifndef _SEED_DIR_H_
#define _SEED_DIR_H_
#include <string>
#include <vector>
#include "core/container/kstring.h"
#include "path.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "core/ref.h"

namespace Seed {
class File;
class Dir : public RefCounted {
    private:
        Path path;

    public:
        static Ref<Dir> open(KStr path);
        Path concat(KStr path) {
            Path new_path = this->path;
            new_path.push(path);
            return new_path;
        }

        Path concat(const KString &path) { return concat(path.to_str()); }

        std::vector<Path> list();

        Path &get_path() { return path; }
        static bool exists(const Path &path) {
#ifdef _WIN32
            struct _stat s;
            _stat(path.data(), &s);
            return (s.st_mode & _S_IFDIR);
#else
            struct stat s;
            stat(path, &s);
            return S_ISDIR(s.st_mode);
#endif
        }
        Ref<File> open_file(const KStr path, const char *mode = "rb");

        /* return false when fail to create */
        static bool create_if_not_exists(const Path &path) {
            if (!exists(path)) {
                int r = _mkdir(path.data());
                /* 0 is succeess */
                return r == 0;
            }
            return true;
        }
};
}  // namespace Seed

#endif