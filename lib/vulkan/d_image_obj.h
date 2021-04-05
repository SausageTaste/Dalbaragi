#pragma once

#include "d_vulkan_header.h"
#include "d_image_parser.h"
#include "d_command.h"


namespace dal {

    class ImageView {

    private:
        VkImageView m_view = VK_NULL_HANDLE;

    public:
        ImageView() = default;
        ImageView(const ImageView&) = delete;
        ImageView& operator=(const ImageView&) = delete;

    public:
        ImageView(
            const VkImage image,
            const VkFormat format,
            const uint32_t mip_level,
            const VkImageAspectFlags aspect_flags,
            const VkDevice logi_device
        );

        ~ImageView();

        ImageView(ImageView&&) noexcept;

        ImageView& operator=(ImageView&&) noexcept;

        [[nodicard]]
        bool init(
            const VkImage image,
            const VkFormat format,
            const uint32_t mip_level,
            const VkImageAspectFlags aspect_flags,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto get() const {
            return this->m_view;
        }

    };


    class TextureImage {

    private:
        VkImage m_image = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

        VkFormat m_format;

    public:
        void init_texture(
            const ImageData& img,
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destory(const VkDevice logi_device);

        auto image() const {
            return this->m_image;
        }

        auto format() const {
            return this->m_format;
        }

    };


    class Sampler {

    private:
        VkSampler m_sampler = VK_NULL_HANDLE;

    public:
        void init_for_color_map(
            const bool enable_anisotropy,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto get() const {
            return this->m_sampler;
        }

    };

}
