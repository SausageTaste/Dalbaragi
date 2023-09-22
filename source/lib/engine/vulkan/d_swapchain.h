#pragma once

#include <limits>
#include <vector>
#include <optional>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "d_indices.h"
#include "d_image_obj.h"
#include "d_sync_primitives.h"


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


    class SwapchainSyncManager {

    private:
        template <typename T, typename _IndexType>
        class FenceSemaphList {

        private:
            std::vector<T> m_list;

        public:
            void init(const size_t count, const VkDevice logi_device) {
                this->destroy(logi_device);
                this->m_list.resize(count);

                for (auto& x : this->m_list)
                    x.init(logi_device);
            }

            void destroy(const VkDevice logi_device) {
                for (auto& x : this->m_list)
                    x.destory(logi_device);

                this->m_list.clear();
            }

            auto& at(const _IndexType& index) const {
                return this->m_list.at(*index);
            }

        };

    public:
        FenceSemaphList<Semaphore, FrameInFlightIndex> m_semaph_img_available;

        FenceSemaphList<Semaphore, FrameInFlightIndex> m_semaph_cmd_done_gbuf;
        FenceSemaphList<Semaphore, FrameInFlightIndex> m_semaph_cmd_done_final;
        FenceSemaphList<Semaphore, FrameInFlightIndex> m_semaph_cmd_done_alpha;

        FenceSemaphList<Fence, FrameInFlightIndex> m_fence_frame_in_flight;

    private:
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

        auto& fence_image_in_flight(const SwapchainIndex& index) {
            return this->m_img_in_flight_fences.at(index.get());
        }

        auto& fence_image_in_flight(const SwapchainIndex& index) const {
            return this->m_img_in_flight_fences.at(index.get());
        }

    };


    struct SwapchainSpec {

    private:
        glm::vec2 m_extent{};
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
            return this->extent().x;
        }

        auto height() const {
            return this->extent().y;
        }

    };


    enum class ImgAcquireResult{ success, fail, out_of_date, suboptimal };


    class SwapchainManager {

    private:
        SwapchainSyncManager m_sync_man;
        std::vector<VkImage> m_images;
        std::vector<ImageView> m_views;
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

        VkFormat m_image_format;
        VkExtent2D m_identity_extent, m_screen_extent;
        VkSurfaceTransformFlagBitsKHR m_transform;

        glm::mat4 m_pre_rotate_mat;
        float m_perspective_ratio;

    public:
        ~SwapchainManager();

        [[nodiscard]]
        bool init(
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

        auto& screen_extent() const {
            return this->m_screen_extent;
        }

        auto& identity_extent() const {
            return this->m_identity_extent;
        }

        auto perspective_ratio() const {
            return this->m_perspective_ratio;
        }

        auto& pre_ratation_mat() const {
            return this->m_pre_rotate_mat;
        }

        auto& views() const {
            return this->m_views;
        }

        auto& sync_man() {
            return this->m_sync_man;
        }

        auto& sync_man() const {
            return this->m_sync_man;
        }

        bool is_format_srgb() const;

        SwapchainSpec make_spec() const;

        std::pair<ImgAcquireResult, SwapchainIndex> acquire_next_img_index(const FrameInFlightIndex& cur_img_index, const VkDevice logi_device) const;

    private:
        void destroy_except_swapchain(const VkDevice logi_device);

    };

}
