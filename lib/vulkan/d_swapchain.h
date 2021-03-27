#pragma once

#include <vector>
#include <optional>
#include <algorithm>

#include "d_image_obj.h"
#include "d_sync_primitives.h"


namespace dal {

    constexpr int MAX_FRAMES_IN_FLIGHT = 2;


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


    class SwapchainSyncManager {

    private:
        std::vector<Semaphore> m_img_available;
        std::vector<Semaphore> m_render_finished;
        std::vector<Fence> m_frame_in_flight_fences;
        std::vector<const Fence*> m_img_in_flight_fences;

    public:
        SwapchainSyncManager() = default;

        SwapchainSyncManager(const SwapchainSyncManager&) = delete;
        SwapchainSyncManager& operator=(const SwapchainSyncManager&) = delete;
        SwapchainSyncManager(SwapchainSyncManager&&) = delete;
        SwapchainSyncManager& operator=(SwapchainSyncManager&&) = delete;

    public:
        void init(const uint32_t swapchain_count, const VkDevice logi_device);

        void destroy(const VkDevice logi_device);

        auto& semaphore_img_available(const size_t index) const {
            return this->m_img_available.at(index);
        }

        auto& semaphore_render_finished(const size_t index) const {
            return this->m_render_finished.at(index);
        }

        auto& fence_frame_in_flight(const size_t index) const {
            return this->m_frame_in_flight_fences.at(index);
        }

        auto& fences_image_in_flight() {
            return this->m_img_in_flight_fences;
        }
        auto& fences_image_in_flight() const {
            return this->m_img_in_flight_fences;
        }

    };


    struct SwapchainSpec {

    private:
        VkExtent2D m_extent{};
        VkFormat m_image_format{};
        uint32_t m_count = 0;

    public:
        bool operator==(const SwapchainSpec& other) const;

        void set(const uint32_t count, const VkFormat format, const VkExtent2D extent);

        auto count() const {
            return this->m_count;
        }

        auto format() const {
            return this->m_image_format;
        }

        auto extent() const {
            return this->m_extent;
        }

        auto width() const {
            return this->extent().width;
        }

        auto height() const {
            return this->extent().height;
        }

    };


    class SwapchainManager {

    private:
        SwapchainSyncManager m_sync_man;
        std::vector<VkImage> m_images;
        std::vector<ImageView> m_views;
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

        VkFormat m_image_format;
        VkExtent2D m_extent;

    public:
        ~SwapchainManager();

        void init(
            const unsigned desired_width,
            const unsigned desired_height,
            const QueueFamilyIndices& indices,
            const VkSurfaceKHR surface,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        uint32_t size() const;

        auto swapchain() const {
            return this->m_swapChain;
        }

        auto format() const {
            return this->m_image_format;
        }

        auto& extent() const {
            return this->m_extent;
        }

        auto& views() const {
            return this->m_views;
        }

        auto& sync_man() const {
            return this->m_sync_man;
        }

        SwapchainSpec make_spec() const;

        uint32_t acquire_next_img_index(const size_t cur_img_index, const VkDevice logi_device) const;

    };

}
