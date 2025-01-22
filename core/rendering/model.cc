#include "model.h"
#include "core/math/mat4.h"

namespace Seed {
Ref<Model> Model::create() {
    Ref<Model> model;
    model.create();
    std::vector<VertexAttribute> vertices_attrs = {
        {.layout_num = 0, .size = 3, .stride = sizeof(Vertex)},
        {.layout_num = 1, .size = 3, .stride = sizeof(Vertex)},
        {.layout_num = 2, .size = 2, .stride = sizeof(Vertex)}};
    std::vector<VertexAttribute> instance_attrs = {
        {
            .layout_num = 3,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 4,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 5,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 6,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
    };
    model->vertices_desc_rc.alloc_vertex_desc(vertices_attrs);
    model->instance_rc.alloc_vertex(sizeof(Mat4), 1, NULL);
    model->instance_desc_rc.alloc_vertex_desc(instance_attrs);
}
}  // namespace Seed