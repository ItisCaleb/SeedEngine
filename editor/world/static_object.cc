#include "static_object.h"

namespace Seed {

Handle ObjectInstanceBatch::insert(u16 id, const Transform &transform) {
    Handle handle =
        instances.insert(ObjectInstance{id, transform.get_model_matrix()});
    mark_dirty();
    return handle;
}
void ObjectInstanceBatch::update(Handle handle, const Transform &transform) {
    if (handle == NULL_HANDLE || handle >= this->instances.size()) {
        return;
    }
    this->instances[handle].world_mat = transform.get_model_matrix();
    mark_dirty();
}
void ObjectInstanceBatch::remove(Handle handle) {
    this->instances.erase(handle);
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