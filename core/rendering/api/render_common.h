#ifndef _SEED_RENDER_COMMON_H_
#define _SEED_RENDER_COMMON_H_

enum class TextureType{
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_CUBEMAP,
    TEXTURE_2D_ARRAY
};

enum class TextureFormat{
    FORMAT_R,
    FORMAT_RG,
    FORMAT_RGB,
    FORMAT_RGBA
};

#endif