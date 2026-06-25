#include "image.h"
#include <spdlog/spdlog.h>
#include "core/macro.h"
#include "core/rendering/render_common.h"
#include "core/types.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define SEED_ARCH_X86
#elif defined(__arm64__) || defined(__aarch64__)
#include <arm_neon.h>
#define SEED_ARCH_ARM
#endif
#include <algorithm>
#include <stb_image.h>
#include <stb_image_write.h>

namespace Seed {

void Image::update(u8 *data, u32 w, u32 h, u32 off_x, u32 off_y) {
    u32 channel = get_pixel_format_size(format);
    u32 offset = (off_x + off_y * width);
    /* resize if overflow */
    if (w * h + offset > width * height) {
        u8 *new_data = (u8 *)realloc(data, (w * h + offset) * channel);
        if (new_data != nullptr) {
            this->data = new_data;
        }
        this->width = off_x + w;
        this->height = off_y + h;
    }
    memcpy((void *)(&this->data[0] + offset * channel), data, w * h * channel);
}

void Image::update(std::vector<u8> &data, u32 w, u32 h, u32 off_x, u32 off_y) {
    this->update(data.data(), w, h, off_x, off_y);
}

Ref<Texture> Image::create_texture(const SamplerProperty &property) {
    Ref<Texture> texture;
    texture.create(TextureType::TEXTURE_2D, width, height, format, property,
                   this->data);
    return texture;
}
Ref<MappableTexture> Image::create_mappable_texture(
    const SamplerProperty &property) {
    Ref<MappableTexture> texture;
    texture.create(TextureType::TEXTURE_2D, width, height, format, this->data,
                   property);
    return texture;
}

void Image::upload(Ref<Texture> texture) {
    EXPECT_NOT_NULL_RET(texture.ptr());
    texture->update(this->data, width, height);
}

void Image::upload(Ref<MappableTexture> texture) {
    EXPECT_NOT_NULL_RET(texture.ptr());
    texture->update(this->data, width, height);
}

void Image::fill(Color color, u32 w, u32 h, u32 off_x, u32 off_y) {
    u32 pixel_size = get_pixel_format_size(format);
    u32 end_x = std::min(off_x + w, width);
    u32 end_y = std::min(off_y + h, height);
    u32 actual_w = end_x - off_x;

    std::vector<u8> row(actual_w * pixel_size);
    u8 channels[4] = {color.r, color.g, color.b, color.a};
    for (u32 x = 0; x < actual_w; x++)
        memcpy(&row[x * pixel_size], channels, pixel_size);

    for (u32 y = off_y; y < end_y; y++) {
        u8 *dst = &data[(y * width + off_x) * pixel_size];
        memcpy(dst, row.data(), row.size());
    }
}

void Image::resize(u32 w, u32 h) {
    u32 pixel_size = get_pixel_format_size(format);
    u8 *new_data = (u8 *)malloc(w * h * pixel_size);
    u32 min_w = std::min(w, this->width);
    u32 min_h = std::min(h, this->height);
    for (u32 i = 0; i < min_h; i++) {
        memcpy(&new_data[i * w * pixel_size],
               &data[i * this->width * pixel_size], min_w * pixel_size);
    }
    free(data);
    this->data = new_data;
    this->width = w;
    this->height = h;
}

void Image::download(Ref<Texture> texture) {
    EXPECT_NOT_NULL_RET(texture.ptr());
}

bool Image::copy_column(Ref<Image> dst, u32 src_x, u32 src_y, u32 dst_x,
                        u32 dst_y, u32 count) {
    if (this->format != dst->format) {
        SEED_WARN("Copying column for different format image, skipping.");
        return false;
    }

    /* check within size */
    if (src_x >= this->width || src_y >= this->height || dst_x >= dst->width ||
        dst_y >= dst->height) {
        return false;
    }
    if (src_y + count > this->height || dst_y + count > dst->height) {
        return false;
    }
    u32 pixel_size = get_pixel_format_size(format);
    for (u32 i = 0; i < count; i++) {
        memcpy(dst->pixel(dst_x, dst_y + i), pixel(src_x, src_y + i),
               pixel_size);
    }
    return true;
}
bool Image::copy_row(Ref<Image> dst, u32 src_x, u32 src_y, u32 dst_x, u32 dst_y,
                     u32 count) {
    if (this->format != dst->format) {
        SEED_WARN("Copying row for different format image, skipping.");
        return false;
    }

    /* check within size */
    if (src_x >= this->width || src_y >= this->height || dst_x >= dst->width ||
        dst_y >= dst->height) {
        return false;
    }
    if (src_x + count > this->width || dst_x + count > dst->width) {
        return false;
    }
    u32 pixel_size = get_pixel_format_size(format);
    memcpy(dst->pixel(dst_x, dst_y), pixel(src_x, src_y), pixel_size * count);
    return true;
}

Ref<Image> Image::median_filter(u32 kernel_size, bool process_alpha) {
    Ref<Image> output;

    if (format != PixelFormat::R && format != PixelFormat::RG &&
        format != PixelFormat::RGB && format != PixelFormat::RGBA) {
        SPDLOG_WARN("Median filter fail, format is not supported.");
        return output;
    }

    std::vector<u8> column_hgs;
    std::vector<u8> kernel_hg;
    column_hgs.resize(256 * width);
    kernel_hg.resize(256);
    i32 r = kernel_size / 2;
    i32 mid = (kernel_size * kernel_size) / 2;
    output.create(format, width, height);
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

#ifdef SEED_ARCH_X86
        // 原有的 AVX 優化實作
        for (i32 i = 0; i < 256; i += 32) {
            __m256i kernel = _mm256_loadu_epi8((const __m256i *)&kernel_hg[i]);
            __m256i column =
                _mm256_loadu_epi8((const __m256i *)&column_hgs[col * 256 + i]);
            __m256i result = _mm256_add_epi8(kernel, column);
            _mm256_storeu_epi8((__m256i *)&kernel_hg[i], result);
        }
#else
        // 通用 C++ 實作（給 ARM/Mac 使用）
        for (i32 i = 0; i < 256; i++) {
            kernel_hg[i] += column_hgs[col * 256 + i];
        }
#endif
    };

    auto update_kernel = [&](i32 to_add, i32 to_sub) {
        if (to_add < 0) to_add = 0;
        if (to_add >= this->width) to_add = this->width - 1;
        if (to_sub < 0) to_sub = 0;
        if (to_sub >= this->width) to_sub = this->width - 1;

#ifdef SEED_ARCH_X86
        for (i32 i = 0; i < 256; i += 32) {
            __m256i kernel = _mm256_loadu_epi8((const __m256i *)&kernel_hg[i]);
            __m256i add_col = _mm256_loadu_epi8(
                (const __m256i *)&column_hgs[to_add * 256 + i]);
            __m256i sub_col = _mm256_loadu_epi8(
                (const __m256i *)&column_hgs[to_sub * 256 + i]);
            __m256i result = _mm256_add_epi8(kernel, add_col);
            result = _mm256_sub_epi8(result, sub_col);
            _mm256_storeu_epi8((__m256i *)&kernel_hg[i], result);
        }
#else
        for (i32 i = 0; i < 256; i++) {
            kernel_hg[i] += column_hgs[to_add * 256 + i];
            kernel_hg[i] -= column_hgs[to_sub * 256 + i];
        }
#endif
    };
    u32 channel = format == PixelFormat::R                        ? 1
                  : format == PixelFormat::RG                     ? 2
                  : format == PixelFormat::RGB                    ? 3
                  : format == PixelFormat::RGBA && !process_alpha ? 3
                                                                  : 4;
    for (u32 i = 0; i < channel; i++) {
        memset(column_hgs.data(), 0, 256 * width);
        memset(kernel_hg.data(), 0, 256);
        /* initiailize column histograms */
        for (u32 col = 0; col < this->width; col++) {
            for (i32 ki = -r - 1; ki < r; ki++) {
                column_hgs[col * 256 + pixel_repeat(col, ki)[i]]++;
            }
        }

        for (i32 row = 0; row < this->height; row++) {
            std::fill(kernel_hg.begin(), kernel_hg.end(), 0);
            for (i32 col = 0; col < this->width; col++) {
                column_hgs[col * 256 + pixel_repeat(col, row - r - 1)[i]]--;
            }
            for (i32 col = 0; col < this->width; col++) {
                column_hgs[col * 256 + pixel_repeat(col, row + r)[i]]++;
            }
            for (i32 col = -r - 1; col < r; col++) {
                add_kernel(col);
            }
            for (i32 col = 0; col < this->width; col++) {
                update_kernel(col + r, col - r - 1);
                output->pixel(col, row)[i] = find_median();
            }
        }
    }

    return output;
}

__attribute__((target("no-avx512f,no-avx512vl,no-avx512bw"))) Ref<Image>
Image::downscale(u32 w, u32 h) {
    if (w >= this->width || h >= this->height) {
        return Ref<Image>(this);
    }
    u32 pixel_size = get_pixel_format_size(format);
    u8 *target = (u8 *)malloc(w * h * pixel_size);
    i32 source_w = (i32)this->width;
    i32 source_h = (i32)this->height;
    f32 h_ratio = (f32)this->width / w;
    f32 w_ratio = (f32)this->height / h;
    for (u32 y = 0; y < h; y++) {
        f32 src_y = ((f32)y + 0.5f) * h_ratio - 0.5f;
        i32 y0 = (i32)std::floor(src_y);
        f32 ty = src_y - y0;
        if (y0 < 0) {
            y0 = 0;
            ty = 0.0f;
        }
        i32 y1 = std::min(y0 + 1, source_h - 1);

        for (u32 x = 0; x < w; x++) {
            float src_x = ((float)x + 0.5f) * w_ratio - 0.5f;
            i32 x0 = (i32)std::floor(src_x);
            float tx = src_x - x0;
            if (x0 < 0) {
                x0 = 0;
                tx = 0.0f;
            }
            i32 x1 = std::min(x0 + 1, source_w - 1);

            for (u32 c = 0; c < pixel_size; c++) {
                f32 c00 = data[(y0 * source_w + x0) * pixel_size + c];
                f32 c10 = data[(y0 * source_w + x1) * pixel_size + c];
                f32 c01 = data[(y1 * source_w + x0) * pixel_size + c];
                f32 c11 = data[(y1 * source_w + x1) * pixel_size + c];
                f32 cx0 = c00 + (c10 - c00) * tx;
                f32 cx1 = c01 + (c11 - c01) * tx;
                f32 value = cx0 + (cx1 - cx0) * ty;
                target[(y * w + x) * pixel_size + c] =
                    (u8)std::clamp((i32)(value + 0.5f), 0, 255);
            }
        }
    }
    Ref<Image> image(new Image(format, w, h, target));
    return image;
}

Ref<Image> Image::load_from_file(const Path &path, bool force_rgba) {
    int w, h, comp;

    void *_data = stbi_load(path.data(), &w, &h, &comp, force_rgba ? 4 : 0);
    if (!_data) return Ref<Image>();
    comp = force_rgba ? 4 : comp;
    PixelFormat format = comp == 1   ? PixelFormat::R
                         : comp == 2 ? PixelFormat::RG
                         : comp == 3 ? PixelFormat::RGB
                                     : PixelFormat::RGBA;
    Ref<Image> image(new Image(format, w, h, _data));
    return image;
}

void Image::save_disk(const Path &path) {
    stbi_write_png(path.data(), width, height, get_pixel_format_size(format),
                   data, 0);
}

Image::Image(PixelFormat format, u32 w, u32 h, void *buffer)
    : format(format), width(w), height(h) {
    this->data = (u8 *)buffer;
}

Image::Image(PixelFormat format, u32 w, u32 h)
    : format(format), width(w), height(h) {
    this->data = (u8 *)malloc(w * h * get_pixel_format_size(format));
}
Image::~Image() { free(this->data); }

}  // namespace Seed