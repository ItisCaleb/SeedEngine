#include "resource.h"
#include "core/io/path.h"
#include "core/resource/resource_loader.h"
#include "core/resource/resource_entry.h"

namespace Seed {
static u32 internal_counter = 0;
Resource::Resource() {}

const Path &Resource::get_path() const {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    if (entry == nullptr) {
        static Path p("internal");
        return p;
    }
    return entry->path;
}

Resource::~Resource() {
    /* Only unregister resource from external*/
    if (!this->uuid.is_null()) {
        System::gResourceLoader->unregister_resource(this);
    }
}

}  // namespace Seed