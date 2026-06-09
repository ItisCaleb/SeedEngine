#ifndef _SEED_EDITOR_WORLD_H_
#define _SEED_EDITOR_WORLD_H_

#include <nlohmann/json.hpp>
#include <vector>
#include "core/container/kstring.h"
#include "core/math/vec3.h"
#include "core/misc/uuid.h"
#include "core/resource/resource_entry.h"
#include "core/types.h"
#include "editor/gui/inspectable.h"
#include "editor_terrain.h"
#include "world_renderer.h"
#include "core/resource/sky.h"

namespace Seed {

struct EditorStaticObject {
        nlohmann::ordered_json raw = nlohmann::ordered_json::object();
        KString name;
        i32 x = 0;
        i32 y = 0;
        UUID model;
};

struct EditorPointLight {
        nlohmann::ordered_json raw = nlohmann::ordered_json::object();
        Vec3 position{};
        Vec3 diffuse{1.0f, 1.0f, 1.0f};
        Vec3 specular{1.0f, 1.0f, 1.0f};
};

struct EditorDirectionalLight {
        nlohmann::ordered_json raw = nlohmann::ordered_json::object();
        Vec3 direction{-0.5f, -0.5f, 0.0f};
        Vec3 diffuse{0.8f, 0.8f, 0.8f};
        Vec3 specular{0.4f, 0.4f, 0.4f};
        bool enabled = true;
};

struct EditorSky {
        nlohmann::ordered_json raw = nlohmann::ordered_json::object();
        UUID up;
        UUID down;
        UUID left;
        UUID right;
        UUID front;
        UUID back;
        Ref<TextureCubemap> cubemap;
        Ref<Sky> sky;
};

struct EditorChunk {
        nlohmann::ordered_json raw = nlohmann::ordered_json::object();
        u32 x = 0;
        u32 y = 0;
        UUID height_map;
        std::vector<EditorPointLight> lights;
        std::vector<EditorStaticObject> static_objects;
};

class WorldEditor;
class WorldRenderer;
class EditorWorld {
        friend WorldEditor;
        friend WorldRenderer;

    private:
        ResourceConfiguration *config = nullptr;
        KString name;
        EditorSky sky;
        EditorDirectionalLight directional_light;
        std::vector<EditorChunk> chunks;
        std::vector<Ref<Image>> heightmaps;
        Ref<EditorTerrain> terrain;
        Ref<Image> default_heightmap;

    public:
        EditorWorld(ResourceConfiguration *config);
        ~EditorWorld() = default;

        void reload();
        void save();

        ResourceConfiguration *get_config() { return config; }
        const KString &get_name() const { return name; }
        void set_name(KStr name) { this->name = name; }
        EditorSky &get_sky() { return sky; }
        EditorDirectionalLight &get_directional_light() {
            return directional_light;
        }
        std::vector<EditorChunk> &get_chunks() { return chunks; }
        const std::vector<EditorChunk> &get_chunks() const { return chunks; }
        void add_new_chunk(u32 x, u32 y);
};

class EditorWorldInspector : public Inspectable {
    private:
        EditorWorld *world;
        void draw_vec3(KStr label, Vec3 &value);

    public:
        EditorWorldInspector(EditorWorld *world);
        virtual void draw_inspector() override;
        virtual void save() override;
};

}  // namespace Seed

#endif
