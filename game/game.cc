#include <seed/engine.h>


int main(void) {
    Seed::SeedEngine *engine = new Seed::SeedEngine(30.0f);
    engine->start();
    return 0;
}