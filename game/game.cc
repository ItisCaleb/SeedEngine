#include <seed/engine.h>
#include <seed/resource.h>

using namespace Seed;

int main(void) {
    SeedEngine *engine = new SeedEngine(60.0f);
    Ref<Texture> tex =
        ResourceLoader::get_instance()->load<Texture>("assets/1.png");

    engine->start();

    return 0;
}