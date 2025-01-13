#include <seed/engine.h>
#include <seed/resource.h>
#include <spdlog/spdlog.h>

using namespace Seed;

int main(void) {
    SeedEngine *engine = new SeedEngine(60.0f);
    ResourceLoader *loader = ResourceLoader::get_instance();
    Ref<Texture> tex = loader->load<Texture>("assets/1.png");
    try{
        Shader s = loader->loadShader("assets/vertex.glsl", "assets/fragment.glsl");
    }catch(std::exception &e){
        spdlog::error("Error loading Shader: {}", e.what());
    }
    engine->start();

    return 0;
}