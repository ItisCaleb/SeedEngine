#ifndef _SEED_ASSET_VIEWER_
#define _SEED_ASSET_VIEWER_
#include "editor/asset/model_loader.h"

namespace Seed {
class ModelRenderer;
class AssetViewer {
    friend ModelRenderer;
    private:
        EditorModel *current_model = nullptr;
        Ref<Model> model;

    public:
        void init();
        void update();
};
}  // namespace Seed

#endif