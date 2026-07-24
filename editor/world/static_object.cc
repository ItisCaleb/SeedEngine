#include "static_object.h"

namespace Seed {
void ObjectInstanceBatch::insert_object(u16 id, Transform &transform) {
    instances.push_back(ObjectInstance{id, transform.get_model_matrix()});
}
void ObjectInstanceBatch::prepare_uploads(
    std::vector<RHI::UpdateBufferInfo> &uploads) {
    RHI::UpdateBufferInfo info =
        RHI::alloc_heap(sizeof(ObjectInstance) * this->instances.size());
    memcpy(info.data, instances.data(), info.size);
    uploads.push_back(info);
}
AABB ObjectInstanceBatch::translate_bounding_box(const AABB &bounding_box,
                                                 u32 i) {
    AABB result = bounding_box.translate(instances[i].world_mat);
    return result;
}
}  // namespace Seed