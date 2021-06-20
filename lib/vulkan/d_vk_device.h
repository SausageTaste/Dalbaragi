#pragma once

#include <set>
#include <array>

#include "d_swapchain.h"


namespace dal {

    constexpr std::array<const char*, 1> VAL_LAYERS_TO_USE = {
        "VK_LAYER_KHRONOS_validation",
    };

    constexpr std::array<const char*, 1> PHYS_DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };


    class PhysDeviceInfo {

    private:
        VkPhysicalDeviceProperties m_properties{};
        VkPhysicalDeviceFeatures m_features{};
        dal::QueueFamilyIndices m_queue_families;
        dal::SwapChainSupportDetails m_swapchain_details;
        std::vector<VkExtensionProperties> m_available_extensions;

    public:
        PhysDeviceInfo() = default;
        PhysDeviceInfo(const PhysDeviceInfo&) = default;
        PhysDeviceInfo& operator=(const PhysDeviceInfo&) = default;

    public:
        PhysDeviceInfo(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device);

        const char* name() const;

        const char* device_type_str() const;

        bool does_support_anisotropic_sampling() const ;

        bool is_usable() const;

        unsigned calc_score() const;

    private:
        template <typename _Iter>
        bool does_support_all_extensions(const _Iter begin, const _Iter end) const {
            return 0 == this->how_many_extensions_not_supported<_Iter>(begin, end);
        }

        template <typename _Iter>
        size_t how_many_extensions_not_supported(const _Iter begin, const _Iter end) const {
            std::set<std::string> required_extensions(begin, end);

            for ( const auto& extension : this->m_available_extensions ) {
                required_extensions.erase(extension.extensionName);
            }

            return required_extensions.size();
        }

    };


    class PhysicalDevice {

    private:
        VkPhysicalDevice m_handle = VK_NULL_HANDLE;

    public:
        PhysicalDevice() = default;
        PhysicalDevice(const PhysicalDevice&) = default;
        PhysicalDevice& operator=(const PhysicalDevice&) = default;

    public:
        PhysicalDevice(const VkPhysicalDevice phys_device);

        VkPhysicalDevice get() const;

        PhysDeviceInfo make_info(const VkSurfaceKHR surface) const;

        VkFormat find_depth_format() const;

    };


    std::vector<PhysicalDevice> get_phys_devices(const VkInstance instance);

    std::pair<PhysicalDevice, PhysDeviceInfo> get_best_phys_device(const VkInstance instance, const VkSurfaceKHR surface, const bool print_info);


    class LogicalDevice {

    private:
        dal::QueueFamilyIndices m_queue_indices;

        VkDevice m_handle = VK_NULL_HANDLE;
        VkQueue m_graphics_queue = VK_NULL_HANDLE;
        VkQueue m_present_queue = VK_NULL_HANDLE;

    public:
        LogicalDevice() = default;
        LogicalDevice(const LogicalDevice&) = delete;
        LogicalDevice& operator=(const LogicalDevice&) = delete;

    public:
        LogicalDevice(LogicalDevice&& other) noexcept;

        LogicalDevice& operator=(LogicalDevice&& other) noexcept;

        ~LogicalDevice();

        void init(const VkSurfaceKHR surface, const PhysicalDevice& phys_device, const PhysDeviceInfo& phys_info);

        void destroy();

        void wait_idle() const;

        auto& get() const {
            return this->m_handle;
        }

        auto& indices() const {
            return this->m_queue_indices;
        }

        auto& queue_graphics() const {
            return this->m_graphics_queue;
        }

        auto& queue_present() const {
            return this->m_present_queue;
        }

    };

}
