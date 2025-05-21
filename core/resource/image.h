#ifndef _SEED_IMAGE_H_
#define _SEED_IMAGE_H_
#include "core/rendering/api/render_common.h"
#include "core/types.h"
#include "core/resource/resource.h"
#include "core/resource/texture.h"
#include <vector>

namespace Seed {
class Image : public Resource {
        ImageFormat format;
        std::vector<u8> data;
        u32 width, height;

    public:
        void update(std::vector<u8> &data, u32 w, u32 h, u32 off_x = 0,
                    u32 off_y = 0);
        void update(u8 *data, u32 w, u32 h, u32 off_x = 0, u32 off_y = 0);
        void fill(Color color, u32 w, u32 h, u32 off_x = 0, u32 off_y = 0);
        void upload(Ref<Texture> texture);
        void download(Ref<Texture> texture);

        Image(ImageFormat format, u32 w, u32 h);
};
}  // namespace Seed

#endif