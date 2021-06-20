#include "d_vk_device.h"

#include <fmt/format.h>

#include "d_logger.h"


namespace {

    VkFormat find_supported_format(
        const std::initializer_list<VkFormat>& candidates,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features,
        const VkPhysicalDevice phys_device
    ) {
        for (const VkFormat format : candidates) {
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

}


// PhysDeviceInfo
namespace dal {

    PhysDeviceInfo::PhysDeviceInfo(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device) {
        vkGetPhysicalDeviceProperties(phys_device, &this->m_properties);
        vkGetPhysicalDeviceFeatures(phys_device, &this->m_features);
        this->m_queue_families.init(surface, phys_device);
        this->m_swapchain_details.init(surface, phys_device);

        {
            uint32_t extension_count;
            vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &extension_count, nullptr);
            this->m_available_extensions.resize(extension_count);
            vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &extension_count, this->m_available_extensions.data());
        }
    }

    const char* PhysDeviceInfo::name() const {
        return this->m_properties.deviceName;
    }

    const char* PhysDeviceInfo::device_type_str() const {
        switch ( this->m_properties.deviceType ) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return "integrated";
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return "discrete";
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return "virtual";
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                return "cpu";
            default:
                return "unknown";
        }
    }

    bool PhysDeviceInfo::does_support_anisotropic_sampling() const {
        return this->m_features.samplerAnisotropy;
    }

    bool PhysDeviceInfo::is_usable() const {
        if (!this->does_support_all_extensions( dal::PHYS_DEVICE_EXTENSIONS.begin(), dal::PHYS_DEVICE_EXTENSIONS.end() ))
            return false;

        if (!this->m_queue_families.is_complete())
            return false;

        if (this->m_swapchain_details.m_formats.empty())
            return false;
        if (this->m_swapchain_details.m_present_modes.empty())
            return false;

        return true;
    }

    unsigned PhysDeviceInfo::calc_score() const {
        if (!this->is_usable())
            return 0;

        unsigned score = 0;
        {
            // Discrete GPUs have a significant performance advantage
            if ( VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == this->m_properties.deviceType )
                score += 5000;

            // Maximum possible size of textures affects graphics quality
            score += this->m_properties.limits.maxImageDimension2D;

            if ( this->m_features.textureCompressionASTC_LDR )
                score += 1000;
        }

        return score;
    }

}


// PhysicalDevice
namespace dal {

    PhysicalDevice::PhysicalDevice(const VkPhysicalDevice phys_device)
        : m_handle(phys_device)
    {

    }

    VkPhysicalDevice PhysicalDevice::get() const {
        dalAssert(VK_NULL_HANDLE != this->m_handle);
        return this->m_handle;
    }

    PhysDeviceInfo PhysicalDevice::make_info(const VkSurfaceKHR surface) const {
        return PhysDeviceInfo{ surface, this->m_handle };
    }

    VkFormat PhysicalDevice::find_depth_format() const {
        return ::find_supported_format(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            this->m_handle
        );
    }


    std::vector<PhysicalDevice> get_phys_devices(const VkInstance instance) {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        std::vector<PhysicalDevice> result;
        result.reserve(device_count);
        for (const auto& x : devices) {
            result.push_back(PhysicalDevice{x});
        }

        return result;
    }

    std::pair<PhysicalDevice, PhysDeviceInfo> get_best_phys_device(const VkInstance instance, const VkSurfaceKHR surface, const bool print_info) {
        const auto phys_devices = dal::get_phys_devices(instance);

        unsigned best_score = 0;
        PhysicalDevice best_device;
        PhysDeviceInfo best_info;

        if (print_info) {
            dalInfo(fmt::format("Physical devices count: {}", phys_devices.size()).c_str());
        }

        for (auto& x : phys_devices) {
            const auto info = x.make_info(surface);
            const auto this_score = info.calc_score();

            if (this_score > best_score) {
                best_score = this_score;
                best_device = x;
                best_info = info;
            }

            if (print_info) {
                dalInfo(fmt::format(" * {} ({}) : {}", info.name(), info.device_type_str(), this_score).c_str());
            }
        }

        return std::make_pair(best_device, best_info);
    }

}


// LogicalDevice
namespace dal {

    LogicalDevice::LogicalDevice(LogicalDevice&& other) noexcept {
        std::swap(this->m_handle, other.m_handle);
        std::swap(this->m_graphics_queue, other.m_graphics_queue);
        std::swap(this->m_present_queue, other.m_present_queue);
    }

    LogicalDevice& LogicalDevice::operator=(LogicalDevice&& other) noexcept {
        std::swap(this->m_handle, other.m_handle);
        std::swap(this->m_graphics_queue, other.m_graphics_queue);
        std::swap(this->m_present_queue, other.m_present_queue);
        return *this;
    }

    LogicalDevice::~LogicalDevice() {
        this->destroy();
    }

    void LogicalDevice::init(const VkSurfaceKHR surface, const PhysicalDevice& phys_device, const PhysDeviceInfo& phys_info) {
        this->m_queue_indices.init(surface, phys_device.get());

        // Create vulkan device
        {
            std::vector<VkDeviceQueueCreateInfo> create_info_queues;
            std::set<uint32_t> unique_queue_families{ this->indices().graphics_family(), this->indices().present_family() };
            const float queuePriority = 1;

            for ( const auto queue_family : unique_queue_families ) {
                VkDeviceQueueCreateInfo create_info{};
                create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                create_info.queueFamilyIndex = queue_family;
                create_info.queueCount = 1;
                create_info.pQueuePriorities = &queuePriority;
                create_info_queues.push_back(create_info);
            }

            VkPhysicalDeviceFeatures device_features{};
            device_features.samplerAnisotropy = phys_info.does_support_anisotropic_sampling();

            VkDeviceCreateInfo create_info_device{};
            create_info_device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info_device.queueCreateInfoCount    = create_info_queues.size();
            create_info_device.pQueueCreateInfos       = create_info_queues.data();
            create_info_device.pEnabledFeatures        = &device_features;
            create_info_device.enabledExtensionCount   = dal::PHYS_DEVICE_EXTENSIONS.size();
            create_info_device.ppEnabledExtensionNames = dal::PHYS_DEVICE_EXTENSIONS.data();
#ifdef DAL_VK_DEBUG
            create_info_device.enabledLayerCount   = dal::VAL_LAYERS_TO_USE.size();
            create_info_device.ppEnabledLayerNames = dal::VAL_LAYERS_TO_USE.data();
#endif

            const auto create_result = vkCreateDevice(phys_device.get(), &create_info_device, nullptr, &this->m_handle);
            dalAssert(VK_SUCCESS == create_result);
        }

        vkGetDeviceQueue(this->m_handle, this->indices().graphics_family(), 0, &this->m_graphics_queue);
        vkGetDeviceQueue(this->m_handle, this->indices().present_family(), 0, &this->m_present_queue);
    }

    void LogicalDevice::destroy() {
        // Queues are destoryed implicitly when the corresponding VkDevice is destroyed.

        if (VK_NULL_HANDLE != this->m_handle) {
            vkDestroyDevice(this->m_handle, nullptr);
            this->m_handle = VK_NULL_HANDLE;
        }
    }

    void LogicalDevice::wait_idle() const {
        vkDeviceWaitIdle(this->m_handle);
    }

}
