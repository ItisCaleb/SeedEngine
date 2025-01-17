#include "core/engine.h"
#include "core/resource.h"
#include "core/rendering/api/render_engine.h"
#include "camera_entity.h"
#include "core/model_entity.h"
#include <spdlog/spdlog.h>

using namespace Seed;

static const Vec3 CUBE[] = {
    /* down-left */
    {-0.5f, -0.5f, -0.5f},
    /* down-right */
    {0.5f, -0.5f, -0.5f},
    /* top-right */
    {0.5f, 0.5f, -0.5f},
    /* top-left */
    {-0.5f, 0.5f, -0.5f},
    /* down-left */
    {-0.5f, -0.5f, 0.5f},
    /* down-right */
    {0.5f, -0.5f, 0.5f},
    /* top-right */
    {0.5f, 0.5f, 0.5f},
    /* top-left */
    {-0.5f, 0.5f, 0.5f},
};

static const u32 CUBE_INDICE[6][6] = {
    // Vertices according to faces
    /* front */
    {0, 1, 2, 2, 3, 0},
    /* back */
    {5, 4, 7, 7, 6, 5},
    /* left */
    {4, 0, 3, 3, 7, 4},
    /* right */
    {1, 5, 6, 6, 2, 1},
    /* down */
    {4, 0, 1, 1, 5, 4},
    /* top */
    {3, 2, 6, 6, 7, 3}};

static const Vec3 CUBE_NORMAL[] = {0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, 1.0f,
                                   -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
                                   0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, 0.0f};

static const Vec2 CUBE_TEX[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                               1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f};

int main(void) {
    SeedEngine *engine = new SeedEngine(60.0f);
    ResourceLoader *loader = ResourceLoader::get_instance();

    Ref<Texture> tex = loader->load<Texture>("assets/1.png");

    std::vector<Vertex> vertices;
    std::vector<Texture> t;

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
           vertices.push_back(Vertex{CUBE[CUBE_INDICE[i][j]], CUBE_NORMAL[i], CUBE_TEX[j]});
        }
    }

    Ref<Mesh> mesh = Mesh::create(vertices, t);

    
    Entity *ent = new ModelEntity(Vec3{0, 0, -2}, mesh);
    ent->set_rotation(Vec3{0.5, 0.5, 0.5});
    engine->get_world()->add_entity<CameraEntity>();
    engine->get_world()->add_entity(ent);

    engine->start();

    return 0;
}