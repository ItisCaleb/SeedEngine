#ifndef _SEED_RESOURCE_H_
#define _SEED_RESOURCE_H_
#include <functional>
#include "core/io/file.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/ref.h"

namespace Seed {

typedef u64 ResourceTypeID;

class ResourceConfiguration;
class Resource : public RefCounted {
    private:
        UUID uuid;

    public:
        const Path& get_path() const;
        void set_uuid(UUID uuid) { this->uuid = uuid; }
        UUID get_uuid() { return uuid; }
        Resource();
        ~Resource();
};

class ResourceLoader;
struct ResourceTypeInfo {
        ResourceTypeID id;
        bool has_data;
        std::function<Ref<Resource>(ResourceLoader &, ResourceConfiguration &,
                                    Ref<File>)>
            load;
        std::function<void(ResourceConfiguration &)> generate_config;
};

}  // namespace Seed

#endif