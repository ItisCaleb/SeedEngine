#ifndef _SEED_EDITOR_WORLD_H_
#define _SEED_EDITOR_WORLD_H_

#include <nlohmann/json.hpp>
#include <vector>
#include "core/container/kstring.h"
#include "core/math/vec3.h"
#include "core/misc/uuid.h"
#include "core/resource/resource_entry.h"
#include "core/resource/world_setting.h"
#include "core/types.h"
#include "editor/gui/inspectable.h"
#include "editor_terrain.h"
#include "world_renderer.h"
#include "core/resource/sky.h"

namespace Seed {

using EditorStaticObject = StaticObjectSetting;
using EditorPointLight = PointLightSetting;
using EditorDirectionalLight = DirectionalLightSetting;
using EditorChunk = ChunkSetting;

struct EditorSky : public SkySetting {
        Ref<TextureCubemap> cubemap;
        Ref<Sky> sky;
};

class WorldEditor;
class WorldRenderer;
class EditorWorld {
        friend WorldEditor;
        friend WorldRenderer;

    private:
        ResourceEntry *entry = nullptr;
        ResourceConfiguration *config = nullptr;
        Ref<WorldSetting> setting;
        EditorSky sky;
        std::vector<Ref<Image>> heightmaps;
        Ref<EditorTerrain> terrain;

    public:
        EditorWorld(ResourceEntry *entry);
        ~EditorWorld() = default;

        void reload();
        void save();
        void apply_directional_light_to_runtime();

        ResourceConfiguration *get_config() { return config; }
        const KString &get_name() const { return setting->name; }
        void set_name(KStr name) { setting->name = name; }
        EditorSky &get_sky() { return sky; }
        EditorDirectionalLight &get_directional_light() {
            return setting->dir_light;
        }
        std::vector<EditorChunk> &get_chunks() { return setting->chunks; }
        const std::vector<EditorChunk> &get_chunks() const {
            return setting->chunks;
        }
        void add_new_chunk(i32 x, i32 y);
        void clear_tiles();
};

class EditorWorldInspector : public Inspectable {
    private:
        EditorWorld *world;
        bool draw_vec3(KStr label, Vec3 &value);

    public:
        EditorWorldInspector(EditorWorld *world);
        virtual void draw_inspector() override;
        virtual void save() override;
};

}  // namespace Seed

#endif
