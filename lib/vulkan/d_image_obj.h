#pragma once

#include "d_vulkan_header.h"


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

}
