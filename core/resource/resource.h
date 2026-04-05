#ifndef _SEED_RESOURCE_H_
#define _SEED_RESOURCE_H_
#include "core/io/path.h"
#include "core/ref.h"

namespace Seed {

class Resource : public RefCounted {
    private:
        Path path;

    public:
        Path &get_path() { return path; }
        void set_path(const Path &path) { this->path = path; }
        Resource();
        ~Resource();
};

}  // namespace Seed

#endif