#include "d_image_obj.h"

#include "d_logger.h"


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
     // viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = mip_level;
     // viewInfo.subresourceRange.baseArrayLayer = 0;
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
