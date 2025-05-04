#include "terrain_renderer.h"
#include "core/resource/resource_loader.h"
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

        vertices_desc.add_attr(0, VertexAttributeType::FLOAT, 2, 0);
        vertices_desc.add_attr(1, VertexAttributeType::FLOAT, 2, 0);
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
        dp.use_vertex(&terrain->vertices, &this->vertices_desc);
        dp.use_texture(terrain->height_map->get_render_resource(), 0);
        dp.render(&terrain_shader, RenderPrimitiveType::PATCHES, 0, false);
        dp.end();
        
    }
    void TerrainRenderer::cleanup(){
        
    }
}