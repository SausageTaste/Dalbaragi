#include "d_image_obj.h"

#include <cstdint>

#include <fmt/format.h>

#include "d_logger.h"
#include "d_buffer_memory.h"


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

    bool has_stencil_component(VkFormat depth_format) {
        return depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    template <typename T>
    uint32_t calc_mip_levels(const T texture_width, const T texture_height) {
        const auto a = std::floor(std::log2(std::max<double>(texture_width, texture_height)));
        return static_cast<uint32_t>(a);
    }


    auto create_image(
        const uint32_t width,
        const uint32_t height,
        const uint32_t mip_levels,
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
        image_info.mipLevels = mip_levels;
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
        if (VK_SUCCESS != vkBindImageMemory(logi_device, image, memory, 0)) {
            dalAbort("failed to bind image and memory!");
        }

        return std::make_pair(image, memory);
    }

    void transition_image_layout(
        const VkImage image,
        const uint32_t mip_levels,
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
        barrier.subresourceRange.levelCount = mip_levels;
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
        const uint32_t mip_level,
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
        region.imageSubresource.mipLevel = mip_level;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

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

    void generate_mipmaps(
        const VkImage image,
        const int32_t tex_width,
        const int32_t tex_height,
        const uint32_t mip_levels,
        dal::CommandPool& cmdPool,
        const VkQueue graphicsQ,
        const VkDevice logiDevice
    ) {
        const auto cmdBuffer = cmdPool.begin_single_time_cmd(logiDevice);
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mip_width = tex_width;
            int32_t mip_height = tex_height;

            for (uint32_t i = 1; i < mip_levels; ++i) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(
                    cmdBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mip_width, mip_height, 1 };
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(
                    cmdBuffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR
                );

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(
                    cmdBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                if (mip_width > 1) mip_width /= 2;
                if (mip_height > 1) mip_height /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mip_levels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }
        cmdPool.end_single_time_cmd(cmdBuffer, graphicsQ, logiDevice);
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

        viewInfo.subresourceRange = {};
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

    VkImageView ImageView::get() const {
        dalAssert(this->is_ready());
        return this->m_view;
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
        this->m_mip_levels = 1;

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
            this->m_mip_levels,
            this->m_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            phys_device, logi_device
        );

        ::transition_image_layout(
            this->m_image,
            this->m_mip_levels,
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
            0,
            cmd_pool,
            graphics_queue,
            logi_device
        );

        staging_buffer.destroy(logi_device);

        ::transition_image_layout(
            this->m_image,
            this->m_mip_levels,
            this->m_format,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            cmd_pool,
            graphics_queue,
            logi_device
        );
    }

    void TextureImage::init_texture_gen_mipmaps(
        const ImageData& img,
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destory(logi_device);
        this->m_format = ::map_to_vk_format(img.format());
        this->m_mip_levels = ::calc_mip_levels(img.width(), img.height());

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
            this->m_mip_levels,
            this->m_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            phys_device, logi_device
        );

        ::transition_image_layout(
            this->m_image,
            this->m_mip_levels,
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
            0,
            cmd_pool,
            graphics_queue,
            logi_device
        );

        staging_buffer.destroy(logi_device);

        ::generate_mipmaps(
            this->image(),
            img.width(),
            img.height(),
            this->m_mip_levels,
            cmd_pool,
            graphics_queue,
            logi_device
        );
    }

    void TextureImage::init_attachment(
        const uint32_t width,
        const uint32_t height,
        const VkFormat format,
        const VkImageUsageFlags usage_flags,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destory(logi_device);
        this->m_format = format;
        this->m_mip_levels = 1;

        std::tie(this->m_image, this->m_memory) = ::create_image(
            width, height,
            this->m_mip_levels,
            this->m_format,
            VK_IMAGE_TILING_OPTIMAL,
            usage_flags,
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

    bool TextureImage::is_ready() const {
        return (VK_NULL_HANDLE != this->m_image) && (VK_NULL_HANDLE != this->m_memory);
    }

}


// Sampler
namespace dal {

    void ISampler::create(const VkSamplerCreateInfo& info, const VkDevice logi_device) {
        this->destroy(logi_device);

        if (VK_SUCCESS != vkCreateSampler(logi_device, &info, nullptr, &this->m_sampler))
            dalAbort("failed to create texture sampler!");
    }

    void ISampler::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_sampler) {
            vkDestroySampler(logi_device, this->m_sampler, nullptr);
            this->m_sampler = VK_NULL_HANDLE;
        }
    }

    void SamplerTexture::init(
        const bool enable_anisotropy,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
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
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

        this->create(samplerInfo, logi_device);
    }

    void SamplerDepth::init(
        const bool enable_anisotropy,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(phys_device, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy    = 1;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.mipLodBias = 0;
        samplerInfo.minLod = 0;
        samplerInfo.maxLod = 1;

        this->create(samplerInfo, logi_device);
    }

}


// TextureUnit
namespace dal {

    TextureUnit::~TextureUnit() {
        this->destroy();
    }

    bool TextureUnit::init(
        dal::CommandPool& cmd_pool,
        const dal::ImageData& img_data,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_logi_device = logi_device;

        this->m_image.init_texture_gen_mipmaps(
            img_data,
            cmd_pool,
            graphics_queue,
            phys_device,
            logi_device
        );

        const auto result_view = this->m_view.init(
            this->m_image.image(),
            this->m_image.format(),
            this->m_image.mip_levels(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            logi_device
        );

        return result_view;
    }

    void TextureUnit::destroy() {
        this->m_image.destory(this->m_logi_device);
        this->m_view.destroy(this->m_logi_device);
    }

    bool TextureUnit::is_ready() const {
        return this->m_image.is_ready() && this->m_view.is_ready();
    }

}


// SamplerManager
namespace dal {

    void SamplerManager::init(
        const bool enable_anisotropy,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_tex_sampler.init(enable_anisotropy, phys_device, logi_device);
        this->m_depth_sampler.init(enable_anisotropy, phys_device, logi_device);
    }

    void SamplerManager::destroy(const VkDevice logi_device) {
        this->m_tex_sampler.destroy(logi_device);
        this->m_depth_sampler.destroy(logi_device);
    }

}
