#include "resource.h"
#include "core/resource/resource_loader.h"
#include <fmt/format.h>


namespace Seed {
    static u32 internal_counter = 0;
    Resource::Resource(){
    }

    Resource::~Resource(){
        /* Only unregister resource from external*/
        if(!this->get_path().empty()){
            ResourceLoader::get_instance()->unregister_resource(this);
        }
    }

}  // namespace Seed