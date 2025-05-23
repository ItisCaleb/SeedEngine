#include "image.h"
#include <spdlog/spdlog.h>
#include "core/macro.h"

namespace Seed {

void Image::update(u8 *data, u32 w, u32 h, u32 off_x, u32 off_y) {
    u32 channel = static_cast<u32>(format);
    u32 offset = (off_x + off_y * width);
    /* resize if overflow */
    if (w * h + offset > width * height) {
        this->data.resize((w * h + offset) * channel);
        this->width = off_x + w;
        this->height = off_y + h;
    }
    memcpy((void *)(&this->data[0] + offset * channel), data, w * h);
}

void Image::update(std::vector<u8> &data, u32 w, u32 h, u32 off_x, u32 off_y) {
    this->update(data.data(), w, h, off_x, off_y);
}
void Image::upload(Ref<Texture> texture) {
    EXPECT_NOT_NULL_RET(texture.ptr());
    texture->update(this->data.data(), width, height);
}

void Image::fill(Color color, u32 w, u32 h, u32 off_x, u32 off_y) {
    for (i32 i = off_y; i < height && i < h; i++) {
        for (i32 j = off_x; j < width && j < w; j++) {
            u32 index = (i * width + j) * static_cast<u32>(format);
            this->data[index] = color.r;
            if (format >= PixelFormat::RG) {
                this->data[index + 1] = color.g;
            }
            if (format >= PixelFormat::RGB) {
                this->data[index + 2] = color.b;
            }
            if (format >= PixelFormat::RGBA) {
                this->data[index + 3] = color.a;
            }
        }
    }
}

void Image::download(Ref<Texture> texture) {
    EXPECT_NOT_NULL_RET(texture.ptr());
}
Image::Image(PixelFormat format, u32 w, u32 h)
    : format(format), width(w), height(h) {
    this->data.resize(w * h * static_cast<u32>(format));
}

}  // namespace Seed