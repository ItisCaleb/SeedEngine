#ifndef _SEED_VULKAN_HELPER_H_
#define _SEED_VULKAN_HELPER_H_
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#include "core/rendering/render_common.h"

namespace Seed {
class VulkanHelper {
    private:
        inline static VkCullModeFlags cull_mode[] = {
            VK_CULL_MODE_NONE, VK_CULL_MODE_FRONT_BIT, VK_CULL_MODE_BACK_BIT,
            VK_CULL_MODE_FRONT_AND_BACK};
        inline static VkPolygonMode poly_mode[] = {
            VK_POLYGON_MODE_POINT, VK_POLYGON_MODE_LINE, VK_POLYGON_MODE_FILL};
        inline static VkCompareOp compare_op[] = {
            VK_COMPARE_OP_NEVER,
            VK_COMPARE_OP_LESS,
            VK_COMPARE_OP_EQUAL,
            VK_COMPARE_OP_LESS_OR_EQUAL,
            VK_COMPARE_OP_GREATER,
            VK_COMPARE_OP_NOT_EQUAL,
            VK_COMPARE_OP_GREATER_OR_EQUAL,
            VK_COMPARE_OP_ALWAYS};

        inline static VkBlendFactor blend_factor[] = {
            VK_BLEND_FACTOR_ZERO,                      // ZERO
            VK_BLEND_FACTOR_ONE,                       // ONE
            VK_BLEND_FACTOR_SRC_COLOR,                 // SRC_COLOR
            VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,       // ONE_MINUS_SRC_COLOR
            VK_BLEND_FACTOR_DST_COLOR,                 // DST_COLOR
            VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,       // ONE_MINUS_DST_COLOR
            VK_BLEND_FACTOR_SRC_ALPHA,                 // SRC_ALPHA
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,       // ONE_MINUS_SRC_ALPHA
            VK_BLEND_FACTOR_DST_ALPHA,                 // DST_ALPHA
            VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,       // ONE_MINUS_DST_ALPHA
            VK_BLEND_FACTOR_CONSTANT_COLOR,            // CONSTANT_COLOR
            VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,  // ONE_MINUS_CONSTANT_COLOR
            VK_BLEND_FACTOR_CONSTANT_ALPHA,            // CONSTANT_ALPHA
            VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,  // ONE_MINUS_CONSTANT_ALPHA
            VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,        // SRC_ALPHA_SATURATE
            VK_BLEND_FACTOR_SRC1_COLOR,                // SRC1_COLOR
            VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,      // ONE_MINUS_SRC1_COLOR
            VK_BLEND_FACTOR_SRC1_ALPHA,                // SRC1_ALPHA
            VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA       // ONE_MINUS_SRC1_ALPHA
        };

        inline static VkFormat pixel_format[] = {VK_FORMAT_R8_UNORM,
                                                 VK_FORMAT_R8G8_UNORM,
                                                 VK_FORMAT_R8G8B8_UNORM,
                                                 VK_FORMAT_R8G8B8A8_UNORM,
                                                 VK_FORMAT_R16G16B16A16_UNORM,
                                                 VK_FORMAT_R16G16B16A16_SINT,
                                                 VK_FORMAT_X8_D24_UNORM_PACK32,
                                                 VK_FORMAT_D24_UNORM_S8_UINT,
                                                 VK_FORMAT_D32_SFLOAT,
                                                 VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                 VK_FORMAT_S8_UINT};
        inline static VkImageType image_type[] = {
            VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D,
            VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D,
            VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_2D};

        inline static VkImageViewType view_type[] = {
            VK_IMAGE_VIEW_TYPE_1D,         VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_VIEW_TYPE_3D,         VK_IMAGE_VIEW_TYPE_CUBE,
            VK_IMAGE_VIEW_TYPE_1D_ARRAY,   VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_IMAGE_VIEW_TYPE_2D};

        inline static VkSamplerAddressMode address_mode[] = {
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
        };

        inline static VkDescriptorType desc_type[] = {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};

        inline static VkPrimitiveTopology topology[] = {
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST};

    public:
        inline static VkPipelineRasterizationStateCreateInfo rasterizer(
            const RenderRasterizerState &state) {
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType =
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.cullMode = cull_mode[(u8)state.cull_mode];
            rasterizer.polygonMode = poly_mode[(u8)state.poly_mode];
            rasterizer.lineWidth = 1.0f;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f;
            rasterizer.depthBiasClamp = 0.0f;
            rasterizer.depthBiasSlopeFactor = 0.0f;
            return rasterizer;
        }

        inline static VkPipelineDepthStencilStateCreateInfo depth_stencil(
            const RenderDepthStencilState &state) {
            VkPipelineDepthStencilStateCreateInfo depth_stencil{};
            depth_stencil.sType =
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil.maxDepthBounds = 1.0f;
            depth_stencil.stencilTestEnable =
                state.stencil_on ? VK_TRUE : VK_FALSE;
            return depth_stencil;
        }

        inline static VkPipelineColorBlendAttachmentState blend_attachment(
            const RenderBlendState &state) {
            VkPipelineColorBlendAttachmentState blend_state{};
            blend_state.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            blend_state.blendEnable = state.blend_on;
            blend_state.srcColorBlendFactor =
                blend_factor[(u8)state.func.src_rgb];  // Optional
            blend_state.dstColorBlendFactor =
                blend_factor[(u8)state.func.dst_rgb];    // Optional
            blend_state.colorBlendOp = VK_BLEND_OP_ADD;  // Optional

            blend_state.srcAlphaBlendFactor =
                blend_factor[(u8)state.func.src_alpha];  // Optional
            blend_state.dstAlphaBlendFactor =
                blend_factor[(u8)state.func.dst_alpha];  // Optional
            blend_state.alphaBlendOp = VK_BLEND_OP_ADD;  // Optional

            return blend_state;
        }

        inline static VkFormat attr_format(VertexAttributeType type, u32 cnt,
                                           bool should_normalize) {
            VkFormat format = VK_FORMAT_UNDEFINED;
            switch (type) {
                case VertexAttributeType::FLOAT:
                    if (cnt == 1) format = VK_FORMAT_R32_SFLOAT;
                    if (cnt == 2) format = VK_FORMAT_R32G32_SFLOAT;
                    if (cnt == 3) format = VK_FORMAT_R32G32B32_SFLOAT;
                    if (cnt == 4) format = VK_FORMAT_R32G32B32A32_SFLOAT;
                    break;
                case VertexAttributeType::INT:
                    if (cnt == 1) format = VK_FORMAT_R32_SINT;
                    if (cnt == 2) format = VK_FORMAT_R32G32_SINT;
                    if (cnt == 3) format = VK_FORMAT_R32G32B32_SINT;
                    if (cnt == 4) format = VK_FORMAT_R32G32B32A32_SINT;
                    break;
                case VertexAttributeType::UNSIGNED:
                    if (cnt == 1) format = VK_FORMAT_R32_UINT;
                    if (cnt == 2) format = VK_FORMAT_R32G32_UINT;
                    if (cnt == 3) format = VK_FORMAT_R32G32B32_UINT;
                    if (cnt == 4) format = VK_FORMAT_R32G32B32A32_UINT;
                    break;
                case VertexAttributeType::SHORT:
                    if (cnt == 1) format = VK_FORMAT_R16_SINT;
                    if (cnt == 2) format = VK_FORMAT_R16G16_SINT;
                    if (cnt == 3) format = VK_FORMAT_R16G16B16_SINT;
                    if (cnt == 4) format = VK_FORMAT_R16G16B16A16_SINT;
                    break;
                case VertexAttributeType::USHORT:
                    if (cnt == 1) format = VK_FORMAT_R16_UINT;
                    if (cnt == 2) format = VK_FORMAT_R16G16_UINT;
                    if (cnt == 3) format = VK_FORMAT_R16G16B16_UINT;
                    if (cnt == 4) format = VK_FORMAT_R16G16B16A16_UINT;
                    break;
                case VertexAttributeType::UNSIGNED_BYTE:
                    if (should_normalize) {
                        if (cnt == 1) format = VK_FORMAT_R8_UNORM;
                        if (cnt == 2) format = VK_FORMAT_R8G8_UNORM;
                        if (cnt == 3) format = VK_FORMAT_R8G8B8_UNORM;
                        if (cnt == 4) format = VK_FORMAT_R8G8B8A8_UNORM;
                    } else {
                        if (cnt == 1) format = VK_FORMAT_R8_UINT;
                        if (cnt == 2) format = VK_FORMAT_R8G8_UINT;
                        if (cnt == 3) format = VK_FORMAT_R8G8B8_UINT;
                        if (cnt == 4) format = VK_FORMAT_R8G8B8A8_UINT;
                    }
                    break;
                default:
                    break;
            }
            if (format == VK_FORMAT_UNDEFINED) {
                spdlog::warn("VertexAttributeType is not supported.");
            }
            return format;
        };


        inline static void vertex_layout(
            VertexLayout *layout, u32 binding,
            std::vector<VkVertexInputAttributeDescription> &attr_desc,
            std::vector<VkVertexInputBindingDescription> &binding_desc) {
            u32 offset = 0;
            VkVertexInputBindingDescription _binding_desc{};
            _binding_desc.binding = binding;
            _binding_desc.inputRate = layout->is_instance()
                                          ? VK_VERTEX_INPUT_RATE_INSTANCE
                                          : VK_VERTEX_INPUT_RATE_VERTEX;
            _binding_desc.stride = layout->get_stride();
            for (const VertexAttribute &desc : layout->get_attrs()) {
                VkVertexInputAttributeDescription _attr_desc{};
                _attr_desc.binding = binding;
                _attr_desc.location = desc.layout_num;
                _attr_desc.offset = offset;
                _attr_desc.format =
                    attr_format(desc.type, desc.size, desc.should_normalized);
                offset += desc.get_type_size() * desc.size;
                attr_desc.push_back(_attr_desc);
            }
            binding_desc.push_back(_binding_desc);
        }

        inline static VkImageType texture_type(TextureType type) {
            return image_type[(u8)type];
        }
        inline static VkFormat texture_format(PixelFormat format) {
            return pixel_format[(u8)format];
        }

        inline static VkImageViewType texture_view_type(TextureType type) {
            return view_type[(u8)type];
        }

        inline static VkFilter filter(SamplerFilter filter) {
            return filter == SamplerFilter::LINEAR ? VK_FILTER_LINEAR
                                                   : VK_FILTER_NEAREST;
        }

        inline static VkSamplerAddressMode wrap_mode(SamplerWrap wrap) {
            return address_mode[(u8)wrap];
        }

        inline static VkSamplerCreateInfo sampler_info(
            const SamplerProperty &property, float maxSamplerAnisotropy) {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = filter(property.mag_filter);
            samplerInfo.minFilter = filter(property.min_filter);
            samplerInfo.addressModeU = wrap_mode(property.wrap_u);
            samplerInfo.addressModeV = wrap_mode(property.wrap_v);
            samplerInfo.addressModeW = wrap_mode(property.wrap_w);
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;
            samplerInfo.maxAnisotropy = maxSamplerAnisotropy;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            return samplerInfo;
        }

        inline static VkDescriptorType descriptor_type(
            ShaderResourceType type) {
            return desc_type[(u8)type];
        }

        inline static VkPrimitiveTopology primitive(RenderPrimitiveType type) {
            return topology[(u8)type];
        }

        inline static VkIndexType index_type(IndexType type) {
            switch (type) {
                case IndexType::UNSIGNED_BYTE:
                    return VK_INDEX_TYPE_UINT8;
                case IndexType::UNSIGNED_SHORT:
                    return VK_INDEX_TYPE_UINT16;
                case IndexType::UNSIGNED_INT:
                default:
                    return VK_INDEX_TYPE_UINT32;
            }
        }

        inline static bool is_depth(PixelFormat format) {
            switch (format) {
                case PixelFormat::D24:
                case PixelFormat::D32:
                case PixelFormat::D24S8:
                case PixelFormat::D32S8:
                    return true;
                default:
                    return false;
            }
        }

        inline static bool is_stencil(PixelFormat format) {
            switch (format) {
                case PixelFormat::S8:
                case PixelFormat::D24S8:
                case PixelFormat::D32S8:
                    return true;
                default:
                    return false;
            }
        }

        inline static VkImageAspectFlags aspect_flag(PixelFormat format) {
            VkImageAspectFlags flag = 0;
            bool depth = is_depth(format);
            bool stencil = is_stencil(format);
            if (!(stencil || depth)) {
                flag = VK_IMAGE_ASPECT_COLOR_BIT;
            }
            if (depth) {
                flag |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if (stencil) {
                flag |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            return flag;
        }

        inline static VkCompareOp compare(CompareOP op) {
            return compare_op[(u8)op];
        }
};
}  // namespace Seed

#endif