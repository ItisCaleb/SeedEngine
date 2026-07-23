#ifndef _SEED_EDITOR_WORLD_STATE_H_
#define _SEED_EDITOR_WORLD_STATE_H_

#include <map>
#include <vector>

#include "core/misc/uuid.h"
#include "core/ref.h"
#include "core/resource/model.h"
#include "core/resource/texture.h"
#include "core/resource/world_setting.h"
#include "core/world/sky.h"

namespace Seed {

class ResourceEntry;

struct EditorSky : public SkySetting {
        Ref<TextureCubemap> cubemap;
        Ref<Sky> sky;
};

struct EditorStaticModel {
        Ref<BasicModel> model;
        Ref<InstanceData> instances;
};

class EditorWorldState {
    private:
        ResourceEntry *entry = nullptr;
        EditorSky sky;
        std::map<UUID, EditorStaticModel> static_models;

    public:
        explicit EditorWorldState(ResourceEntry *entry) : entry(entry) {}

        Ref<WorldSetting> load() const;
        void save(const WorldSetting &setting) const;

        void load_sky(const SkySetting &setting);
        void write_sky_setting(SkySetting &setting) const;
        void update_skybox_face(UUID uuid, CubemapFace face);
        void apply_directional_light(
            const DirectionalLightSetting &setting) const;
        void rebuild_static_models(const std::vector<ChunkSetting> &chunks);

        EditorSky &get_sky() { return sky; }
        const std::map<UUID, EditorStaticModel> &get_static_models() const {
            return static_models;
        }
};

}  // namespace Seed

#endif
