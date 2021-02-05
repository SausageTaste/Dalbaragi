#pragma once

#include <vector>
#include <algorithm>

#include "d_vulkan_header.h"


namespace dal {

    class QueueFamilyIndices {

    private:
        static constexpr uint32_t NULL_VAL = -1;

    private:
        uint32_t m_graphics_family = NULL_VAL;
        uint32_t m_present_family = NULL_VAL;

    public:
        QueueFamilyIndices() = default;

        QueueFamilyIndices(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device) {
            this->init(surface, phys_device);
        }

        void init(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device);

        bool is_complete(void) const noexcept;

        uint32_t graphics_family(void) const noexcept {
            return this->m_graphics_family;
        }

        uint32_t present_family(void) const noexcept {
            return this->m_present_family;
        }

    };


    class SwapChainSupportDetails {

    public:
        VkSurfaceCapabilitiesKHR m_capabilities;
        std::vector<VkSurfaceFormatKHR> m_formats;
        std::vector<VkPresentModeKHR> m_present_modes;

    public:
        SwapChainSupportDetails() = default;

        SwapChainSupportDetails(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device) {
            this->init(surface, phys_device);
        }

        void init(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device);

    };


    class SwapchainManager {

    private:
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

        VkFormat m_image_format;
        VkExtent2D m_extent;

    public:
        ~SwapchainManager();

        void init(
            const unsigned desired_width,
            const unsigned desired_height,
            const VkSurfaceKHR surface,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

    };

}
