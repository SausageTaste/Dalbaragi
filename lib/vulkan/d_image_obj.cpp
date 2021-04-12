#include "d_image_obj.h"

#include <vector>
#include <cstdint>

#include <fmt/format.h>

#include "d_logger.h"
#include "d_buffer_memory.h"
#include "d_image_parser.h"


namespace {

    VkFormat map_to_vk_format(const dal::ImageFormat format) {
        switch (format) {
            case dal::ImageFormat::r8g8b8a8_srgb:
                return VK_FORMAT_R8G8B8A8_SRGB;
            default:
                dalAbort(fmt::format("Unkown dal::ImageFormat value: {}", static_cast<int>(format)).c_str());
        }
    }

    VkMemoryRequirements get_image_mem_requirements(const VkImage image, const VkDevice logi_device) {
        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(logi_device, image, &mem_requirements);
        return mem_requirements;
    }

    VkFormat find_supported_format(
        const std::vector<VkFormat>& candidates,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features,
        const VkPhysicalDevice phys_device
    ) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(phys_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        dalAbort("failed to find supported format!");
    }

    VkFormat find_depth_format(const VkPhysicalDevice phys_device) {
        return ::find_supported_format(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            phys_device
        );
    }

    bool has_stencil_component(VkFormat depth_format) {
        return depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    auto create_image(
        const uint32_t width,
        const uint32_t height,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage,
        const VkMemoryPropertyFlags properties,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = format;
        image_info.tiling = tiling;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.flags = 0;

        if (VK_SUCCESS != vkCreateImage(logi_device, &image_info, nullptr, &image)) {
            dalAbort("failed to create image!");
        }

        const auto mem_req = ::get_image_mem_requirements(image, logi_device);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = dal::find_memory_type(
            mem_req.memoryTypeBits, properties, phys_device
        );

        if (VK_SUCCESS != vkAllocateMemory(logi_device, &alloc_info, nullptr, &memory)) {
            dalAbort("failed to allocate image memory!");
        }

        vkBindImageMemory(logi_device, image, memory, 0);
        return std::make_pair(image, memory);
    }

    void transition_image_layout(
        const VkImage image,
        const VkFormat format,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkDevice logi_device
    ) {
        auto cmd_buf = cmd_pool.begin_single_time_cmd(logi_device);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags src_stage;
        VkPipelineStageFlags dst_stage;
        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            dalAbort("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            cmd_buf,
            src_stage,
            dst_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        cmd_pool.end_single_time_cmd(cmd_buf, graphics_queue, logi_device);
    }

    void copy_buffer_to_image(
        const VkImage dst_image,
        const VkBuffer src_buffer,
        const uint32_t width,
        const uint32_t height,
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkDevice logi_device
    ) {
        auto cmd_buf = cmd_pool.begin_single_time_cmd(logi_device);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            cmd_buf,
            src_buffer,
            dst_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        cmd_pool.end_single_time_cmd(cmd_buf, graphics_queue, logi_device);
    }

}


// ImageView
namespace dal {

    ImageView::ImageView(
        const VkImage image,
        const VkFormat format,
        const uint32_t mip_level,
        const VkImageAspectFlags aspect_flags,
        const VkDevice logi_device
    ) {
        const auto result = this->init(image, format, mip_level, aspect_flags, logi_device);
        dalAssert(result);
    }

    ImageView::~ImageView() {
        dalAssert(VK_NULL_HANDLE == this->m_view);
    }

    ImageView::ImageView(ImageView&& other) noexcept {
        std::swap(this->m_view, other.m_view);
    }

    ImageView& ImageView::operator=(ImageView&& other) noexcept {
        std::swap(this->m_view, other.m_view);
        return *this;
    }

    bool ImageView::init(
        const VkImage image,
        const VkFormat format,
        const uint32_t mip_level,
        const VkImageAspectFlags aspect_flags,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image    = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format   = format;

        viewInfo.subresourceRange.aspectMask     = aspect_flags;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = mip_level;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

     // viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
     // viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
     // viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
     // viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        return VK_SUCCESS == vkCreateImageView(logi_device, &viewInfo, nullptr, &this->m_view);
    }

    void ImageView::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_view) {
            vkDestroyImageView(logi_device, this->m_view, nullptr);
            this->m_view = VK_NULL_HANDLE;
        }
    }

}


// TextureImage
namespace dal {

    void TextureImage::init_texture(
        const ImageData& img,
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destory(logi_device);
        this->m_format = ::map_to_vk_format(img.format());

        BufferMemory staging_buffer;
        const auto staging_result = staging_buffer.init(
            img.data_size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            phys_device, logi_device
        );
        dalAssert(staging_result);
        staging_buffer.copy_from_mem(img.data(), img.data_size(), logi_device);

        std::tie(this->m_image, this->m_memory) = ::create_image(
            img.width(),
            img.height(),
            this->m_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            phys_device, logi_device
        );

        ::transition_image_layout(
            this->m_image,
            this->m_format,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            cmd_pool,
            graphics_queue,
            logi_device
        );

        ::copy_buffer_to_image(
            this->m_image,
            staging_buffer.buffer(),
            img.width(),
            img.height(),
            cmd_pool,
            graphics_queue,
            logi_device
        );

        staging_buffer.destroy(logi_device);

        ::transition_image_layout(
            this->m_image,
            this->m_format,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            cmd_pool,
            graphics_queue,
            logi_device
        );
    }

    void TextureImage::init_depth(
        const uint32_t width,
        const uint32_t height,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_format = ::find_depth_format(phys_device);

        std::tie(this->m_image, this->m_memory) = ::create_image(
            width, height,
            this->m_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            phys_device, logi_device
        );
    }

    void TextureImage::destory(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_image) {
            vkDestroyImage(logi_device, this->m_image, nullptr);
            this->m_image = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_memory) {
            vkFreeMemory(logi_device, this->m_memory, nullptr);
            this->m_memory = VK_NULL_HANDLE;
        }
    }

}


// Sampler
namespace dal {

    void Sampler::init_for_color_map(
        const bool enable_anisotropy,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(phys_device, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = enable_anisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy    = enable_anisotropy ? properties.limits.maxSamplerAnisotropy : 1;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0;
        samplerInfo.minLod = 0;
        samplerInfo.maxLod = 0;

        if (VK_SUCCESS != vkCreateSampler(logi_device, &samplerInfo, nullptr, &this->m_sampler)) {
            dalAbort("failed to create texture sampler!");
        }
    }

    void Sampler::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_sampler) {
            vkDestroySampler(logi_device, this->m_sampler, nullptr);
            this->m_sampler = VK_NULL_HANDLE;
        }
    }

}


// TextureManager
namespace dal {

    void TextureManager::init(
        dal::filesystem::AssetManager& asset_man,
        CommandPool& cmd_pool,
        const bool enable_anisotropy,
        const VkQueue m_graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_asset_man = &asset_man;
        this->m_cmd_pool = &cmd_pool,
        this->m_graphics_queue = m_graphics_queue;
        this->m_phys_device = phys_device;
        this->m_logi_device = logi_device;

        this->m_tex_sampler.init_for_color_map(enable_anisotropy, this->m_phys_device, this->m_logi_device);
    }

    void TextureManager::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_textures) {
            x.second.m_image.destory(logi_device);
            x.second.m_view.destroy(logi_device);
        }
        this->m_textures.clear();

        this->m_tex_sampler.destroy(logi_device);
    }

    const TextureManager::TextureUnit& TextureManager::request_asset_tex(const dal::filesystem::ResPath& respath) {
        const auto resolved_respath = dal::filesystem::resolve_respath(respath);
        const auto path_str = resolved_respath->make_str();

        const auto result = this->m_textures.find(path_str);
        if (this->m_textures.end() != result) {
            return result->second;
        }
        else {
            auto file = this->m_asset_man->open(resolved_respath.value());
            if (!file->is_ready()) {
                dalAbort(fmt::format("Failed to find texture file: {}", path_str).c_str());
            }
            const auto file_data = file->read_stl<std::vector<uint8_t>>();
            const auto image = dal::parse_image_stb(file_data->data(), file_data->size());

            this->m_textures[path_str] = TextureUnit{};
            auto iter = this->m_textures.find(path_str);

            iter->second.m_image.init_texture(
                image.value(),
                *this->m_cmd_pool,
                this->m_graphics_queue,
                this->m_phys_device,
                this->m_logi_device
            );

            iter->second.m_view.init(
                iter->second.m_image.image(),
                iter->second.m_image.format(),
                1,
                VK_IMAGE_ASPECT_COLOR_BIT,
                m_logi_device
            );

            return iter->second;
        }
    };

}
