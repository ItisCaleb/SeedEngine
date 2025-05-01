#ifndef _SEED_RESOURCE_H_
#define _SEED_RESOURCE_H_
#include "core/ref.h"

#include "core/types.h"

#include <string>

namespace Seed {

class Resource : public RefCounted {
   private:
    std::string path;

   public:
    std::string &get_path() { return path; }
    void set_path(const std::string &path) { this->path = path; }
    Resource();
    ~Resource();
};


}  // namespace Seed

#endif