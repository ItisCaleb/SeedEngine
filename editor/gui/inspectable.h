#ifndef _SEED_INSPECTABLE_H_
#define _SEED_INSPECTABLE_H_

#include "core/container/kstring.h"
#include "core/misc/uuid.h"
#include "core/resource/resource_entry.h"
namespace Seed {
class Inspectable {
    protected:
        ResourceConfiguration *config = nullptr;
        bool drag_uuid(KStr name, UUID &uuid);

    public:
        Inspectable() = default;
        Inspectable(ResourceConfiguration *config) : config(config) {}
        virtual void draw_inspector() {}
        virtual void save() {};
};

class Inspector {

    public:
        void update();
};

}  // namespace Seed

#endif