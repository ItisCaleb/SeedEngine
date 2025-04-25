#include "terrain_renderer.h"
#include "core/resource.h"
#include <spdlog/spdlog.h>


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
    }

    void TerrainRenderer::preprocess(){

    }

    void TerrainRenderer::process(RenderCommandDispatcher &dp, u64 sort_key){
        
    }
    void TerrainRenderer::cleanup(){
        
    }
}