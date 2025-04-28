#include "terrain_renderer.h"
#include "core/resource.h"
#include <spdlog/spdlog.h>
#include "core/engine.h"


namespace Seed{
    void TerrainRenderer::init(){
        ResourceLoader *loader = ResourceLoader::get_instance();

        try {
            this->terrain_shader =
                loader->loadShader("assets/terrain.vert", "assets/terrain.frag", "", "assets/terrain.tessctl", "assets/terrain.tesseval");
        } catch (std::exception &e) {
            SPDLOG_ERROR("Error loading Shader: {}", e.what());
            exit(1);
        }
        std::vector<VertexAttribute> vertices_attrs = {
            {.layout_num = 0, .size = 3, .stride = sizeof(TerrainVertex)},
            {.layout_num = 1, .size = 2, .stride = sizeof(TerrainVertex)}};
        vertices_desc_rc.alloc_vertex_desc(vertices_attrs);
        auto model = Mat4::translate_mat({0, 0, 0}).transpose();
        model_const_rc.alloc_constant("TerrainMatrices", sizeof(Mat4), &model);
    }

    void TerrainRenderer::preprocess(){
        
    }

    void TerrainRenderer::process(RenderCommandDispatcher &dp, u64 sort_key){
        Ref<Terrain> terrain = SeedEngine::get_instance()->get_world()->get_terrain();
        if(terrain.is_null()){
            return;
        }
        dp.begin(sort_key);
        dp.use(&terrain->vertices);
        dp.use(&vertices_desc_rc);
        dp.use(terrain->height_map->get_render_resource(), 0);
        dp.render(&terrain_shader, RenderPrimitiveType::PATCHES, 0, false);
        dp.end();
        
    }
    void TerrainRenderer::cleanup(){
        
    }
}