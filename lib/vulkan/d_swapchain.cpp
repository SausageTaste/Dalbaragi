#include "d_swapchain.h"

#include "d_logger.h"


namespace {

    template <typename T>
    constexpr T clamp(const T v, const T minv, const T maxv) {
        return std::max<T>(minv, std::min<T>(maxv, v));
    }


    VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
        for ( const auto& format : available_formats ) {
            if ( format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
                return format;
            }
        }

        return available_formats[0];
    }

    VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {
        for ( const auto& present_mode : available_present_modes ) {
            if ( VK_PRESENT_MODE_MAILBOX_KHR == present_mode ) {
                return present_mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, const unsigned w, const unsigned h) {
        if ( capabilities.currentExtent.width != UINT32_MAX ) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D extent = { w, h };

            extent.width = ::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = ::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return extent;
        }
    }

}


namespace dal {

    void QueueFamilyIndices::init(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device) {
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, queue_families.data());

        for (size_t i = 0; i < queue_families.size(); ++i) {
            const auto& queue_family = queue_families[i];

            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                this->m_graphics_family = i;

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys_device, i, surface, &present_support);
            if (present_support)
                this->m_present_family = i;

            if (this->is_complete())
                break;
        }
    }

    bool QueueFamilyIndices::is_complete(void) const noexcept {
        if (this->NULL_VAL == this->m_graphics_family)
            return false;
        if (this->NULL_VAL == this->m_present_family)
            return false;

        return true;
    }


    void SwapChainSupportDetails::init(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device) {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, surface, &this->m_capabilities);

        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &format_count, nullptr);
        if (0 != format_count) {
            m_formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &format_count, m_formats.data());
        }

        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, surface, &present_mode_count, nullptr);
        if (0 != present_mode_count) {
            m_present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, surface, &present_mode_count, m_present_modes.data());
        }
    }

}


namespace dal {

    SwapchainManager::~SwapchainManager() {
        dalAssert(VK_NULL_HANDLE == this->m_swapChain);
    }

    void SwapchainManager::init(
        const unsigned desired_width,
        const unsigned desired_height,
        const VkSurfaceKHR surface,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        const SwapChainSupportDetails swapchain_support(surface, phys_device);

        const VkSurfaceFormatKHR surfaceFormat = ::choose_surface_format(swapchain_support.m_formats);
        const VkPresentModeKHR presentMode = ::choose_present_mode(swapchain_support.m_present_modes);
        const VkExtent2D extent = ::choose_extent(swapchain_support.m_capabilities, desired_width, desired_height);

        // Simply sticking to this minimum means that we may sometimes have to wait on the driver to complete
        // internal operations before we can acquire another image to render to. Therefore it is recommended to
        // request at least one more image than the minimum.
        uint32_t imageCount = swapchain_support.m_capabilities.minImageCount + 1;
        if (swapchain_support.m_capabilities.maxImageCount > 0 && imageCount > swapchain_support.m_capabilities.maxImageCount) {
            imageCount = swapchain_support.m_capabilities.maxImageCount;
        }

        // Create swap chain
        {
            VkSwapchainCreateInfoKHR create_info_swapchain{};
            create_info_swapchain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            create_info_swapchain.surface = surface;
            create_info_swapchain.minImageCount = imageCount;
            create_info_swapchain.imageFormat = surfaceFormat.format;
            create_info_swapchain.imageColorSpace = surfaceFormat.colorSpace;
            create_info_swapchain.imageExtent = extent;
            create_info_swapchain.imageArrayLayers = 1;
            create_info_swapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            const QueueFamilyIndices indices{ surface, phys_device };
            uint32_t queueFamilyIndices[] = { indices.graphics_family(), indices.present_family() };

            if (indices.graphics_family() != indices.present_family()) {
                create_info_swapchain.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info_swapchain.queueFamilyIndexCount = 2;
                create_info_swapchain.pQueueFamilyIndices = queueFamilyIndices;
            }
            else {
                create_info_swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                create_info_swapchain.queueFamilyIndexCount = 0; // Optional
                create_info_swapchain.pQueueFamilyIndices = nullptr; // Optional
            }

            create_info_swapchain.preTransform = swapchain_support.m_capabilities.currentTransform;
            create_info_swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            create_info_swapchain.presentMode = presentMode;
            create_info_swapchain.clipped = VK_TRUE;
            create_info_swapchain.oldSwapchain = VK_NULL_HANDLE;

            const auto create_result_swapchain = vkCreateSwapchainKHR(logi_device, &create_info_swapchain, nullptr, &this->m_swapChain);
            dalAssert(VK_SUCCESS == create_result_swapchain);
        }

        this->m_image_format = surfaceFormat.format;
        this->m_extent = extent;
    }

    void SwapchainManager::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_swapChain) {
            vkDestroySwapchainKHR(logi_device, this->m_swapChain, nullptr);
            this->m_swapChain = VK_NULL_HANDLE;
        }
    }

}
