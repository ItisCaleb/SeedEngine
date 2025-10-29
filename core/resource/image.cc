#include "image.h"
#include <spdlog/spdlog.h>
#include "core/macro.h"
#include <algorithm>
#include <immintrin.h>

namespace Seed {

void Image::update(u8 *data, u32 w, u32 h, u32 off_x, u32 off_y) {
    u32 channel = get_pixel_format_size(format);
    u32 offset = (off_x + off_y * width);
    /* resize if overflow */
    if (w * h + offset > width * height) {
        this->data.resize((w * h + offset) * channel);
        this->width = off_x + w;
        this->height = off_y + h;
    }
    memcpy((void *)(&this->data[0] + offset * channel), data, w * h * channel);
}

void Image::update(std::vector<u8> &data, u32 w, u32 h, u32 off_x, u32 off_y) {
    this->update(data.data(), w, h, off_x, off_y);
}

Ref<Texture> Image::create_texture() {
    Ref<Texture> texture;
    texture.create(TextureType::TEXTURE_2D, width, height, format,
                   this->data.data());
    return texture;
}
void Image::upload(Ref<Texture> texture) {
    EXPECT_NOT_NULL_RET(texture.ptr());
    texture->update(this->data.data(), width, height);
}

void Image::fill(Color color, u32 w, u32 h, u32 off_x, u32 off_y) {
    for (i32 i = off_y; i < height && i < h; i++) {
        for (i32 j = off_x; j < width && j < w; j++) {
            u32 index = (i * width + j) * get_pixel_format_size(format);
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

u8 *Image::pixel(u32 x, u32 y) {
    return &this->data[(y * width + x) * get_pixel_format_size(format)];
}

u8 *Image::pixel_repeat(i32 x, i32 y) {
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height - 1;
    return &this->data[(y * width + x) * get_pixel_format_size(format)];
}

Ref<Image> Image::median_filter(u32 kernel_size) {
    std::vector<u8> column_hgs;
    std::vector<u8> kernel_hg;
    column_hgs.resize(256 * width);
    kernel_hg.resize(256);
    Ref<Image> output;
    i32 r = kernel_size / 2;
    i32 mid = (kernel_size * kernel_size) / 2;
    output.create(format, width, height);
    auto t1 = std::chrono::steady_clock::now();
    auto find_median = [&]() -> u8 {
        u32 cnt = 0;
        for (u32 i = 0; i < 255; i++) {
            cnt += kernel_hg[i];
            if (cnt > mid) return i;
        }
        return 0;
    };

    auto add_kernel = [&](i32 col) {
        if (col < 0) col = 0;
        if (col >= this->width) col = this->width - 1;
        for (i32 i = 0; i < 256; i += 32) {
            __m256i kernel = _mm256_loadu_epi8(&kernel_hg[i]);
            __m256i column = _mm256_loadu_epi8(&column_hgs[col * 256 + i]);
            __m256i result = _mm256_add_epi8(kernel, column);
            _mm256_storeu_epi8(&kernel_hg[i], result);
        }
    };

    auto update_kernel = [&](i32 to_add, i32 to_sub) {
        if (to_add < 0) to_add = 0;
        if (to_add >= this->width) to_add = this->width - 1;
        if (to_sub < 0) to_sub = 0;
        if (to_sub >= this->width) to_sub = this->width - 1;
        for (i32 i = 0; i < 256; i += 32) {
            __m256i kernel = _mm256_loadu_epi8(&kernel_hg[i]);
            __m256i add_col = _mm256_loadu_epi8(&column_hgs[to_add * 256 + i]);
            __m256i sub_col = _mm256_loadu_epi8(&column_hgs[to_sub * 256 + i]);
            __m256i result = _mm256_add_epi8(kernel, add_col);
            result = _mm256_sub_epi8(result, sub_col);
            _mm256_storeu_epi8(&kernel_hg[i], result);
        }
    };

    /* initiailize column histograms */
    for (u32 col = 0; col < this->width; col++) {
        for (i32 ki = -r - 1; ki < r; ki++) {
            column_hgs[col * 256 + pixel_repeat(col, ki)[0]]++;
        }
    }

    for (i32 row = 0; row < this->height; row++) {
        std::fill(kernel_hg.begin(), kernel_hg.end(), 0);
        for (i32 col = 0; col < this->width; col++) {
            column_hgs[col * 256 + pixel_repeat(col, row - r - 1)[0]]--;
            column_hgs[col * 256 + pixel_repeat(col, row + r)[0]]++;
        }
        for (i32 col = -r - 1; col < r; col++) {
            add_kernel(col);
        }
        for (i32 col = 0; col < this->width; col++) {
            update_kernel(col + r, col - r - 1);
            output->pixel(col, row)[0] = find_median();
        }
    }
    auto t2 = std::chrono::steady_clock::now();
    fmt::println(
        "duration:{}",
        std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());

    return output;
}

Image::Image(PixelFormat format, u32 w, u32 h)
    : format(format), width(w), height(h) {
    this->data.resize(w * h * get_pixel_format_size(format));
}

}  // namespace Seed