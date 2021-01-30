#include "d_vulkan_man.h"

#include <set>
#include <array>
#include <string>
#include <cstring>
#include <stdexcept>

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include <vulkan/vulkan.h>

#include "d_logger.h"


//#define DAL_VK_DEBUG


namespace {

    class QueueFamilyIndices {

    private:
        static constexpr uint32_t NULL_VAL = -1;

    private:
        uint32_t m_graphics_family = NULL_VAL;
        uint32_t m_present_family = NULL_VAL;

    public:
        bool is_complete(void) const noexcept {
            if (this->NULL_VAL == this->m_graphics_family)
                return false;
            if (this->NULL_VAL == this->m_present_family)
                return false;

            return true;
        }

        uint32_t graphics_family(void) const noexcept {
            return this->m_graphics_family;
        }
        uint32_t present_family(void) const noexcept {
            return this->m_present_family;
        }

        void set_graphics_family(const uint32_t v) noexcept {
            this->m_graphics_family = v;
        }
        void set_present_family(const uint32_t v) noexcept {
            this->m_present_family = v;
        }

    };

    QueueFamilyIndices find_queue_families(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device) {
        QueueFamilyIndices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, queue_families.data());

        for (size_t i = 0; i < queue_families.size(); ++i) {
            const auto& queue_family = queue_families[i];

            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.set_graphics_family(i);

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys_device, i, surface, &present_support);
            if (present_support)
                indices.set_present_family(i);

            if (indices.is_complete())
                break;
        }

        return indices;
    }

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR m_capabilities;
        std::vector<VkSurfaceFormatKHR> m_formats;
        std::vector<VkPresentModeKHR> m_present_modes;
    };

    SwapChainSupportDetails query_swapchain_support(const VkSurfaceKHR surface, const VkPhysicalDevice device) {
        SwapChainSupportDetails result;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &result.m_capabilities);

        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
        if (0 != format_count) {
            result.m_formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, result.m_formats.data());
        }

        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
        if (0 != present_mode_count) {
            result.m_present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, result.m_present_modes.data());
        }

        return result;
    }


    class PhysicalDevice {

    private:
        const std::array<const char*, 1> DEVICE_EXTENSIONS = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

    private:
        VkPhysicalDevice m_handle = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties m_properties;
        VkPhysicalDeviceFeatures m_features;
        QueueFamilyIndices m_queue_families;
        SwapChainSupportDetails m_swapchain_details;
        std::vector<VkExtensionProperties> m_available_extensions;

    public:
        PhysicalDevice() = default;

        PhysicalDevice(const VkPhysicalDevice phys_device, const VkSurfaceKHR surface)
            : m_handle(phys_device)
        {
            vkGetPhysicalDeviceProperties(this->m_handle, &this->m_properties);
            vkGetPhysicalDeviceFeatures(this->m_handle, &this->m_features);
            this->m_queue_families = ::find_queue_families(surface, this->m_handle);
            this->m_swapchain_details = ::query_swapchain_support(surface, this->m_handle);

            {
                uint32_t extension_count;
                vkEnumerateDeviceExtensionProperties(this->m_handle, nullptr, &extension_count, nullptr);
                this->m_available_extensions.resize(extension_count);
                vkEnumerateDeviceExtensionProperties(this->m_handle, nullptr, &extension_count, this->m_available_extensions.data());
            }
        }

        auto name() const {
            return this->m_properties.deviceName;
        }
        auto device_type_str() const {
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

        bool is_usable() const {
            if (!this->m_features.geometryShader)
                return false;
            if (!this->m_features.samplerAnisotropy)
                return false;

            if (!this->does_support_all_extensions( this->DEVICE_EXTENSIONS.begin(), this->DEVICE_EXTENSIONS.end() ))
                return false;

            if (!this->m_queue_families.is_complete())
                return false;

            if (this->m_swapchain_details.m_formats.empty())
                return false;
            if (this->m_swapchain_details.m_present_modes.empty())
                return false;

            return true;
        }

        unsigned calc_score() const {
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


    std::vector<PhysicalDevice> get_phys_devices(const VkInstance instance, const VkSurfaceKHR surface) {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        std::vector<PhysicalDevice> result;
        result.reserve(device_count);
        for (const auto& x : devices) {
            result.push_back(::PhysicalDevice{x, surface});
        }

        return result;
    }

}


namespace {

    constexpr std::array<const char*, 1> VAL_LAYERS_TO_USE = {
        "VK_LAYER_KHRONOS_validation",
    };


    VKAPI_ATTR VkBool32 VKAPI_CALL callback_vk_debug(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        switch ( messageSeverity ) {

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return VK_FALSE;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            break;
        default:
            break;

        }

        const auto err_msg = std::string{ "Vulkan Debug: " } + pCallbackData->pMessage;
        return VK_FALSE;
    }


    VkApplicationInfo make_info_vulkan_app(const char* const window_title) {
        VkApplicationInfo appInfo = {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = window_title;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Dalbaragi";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        return appInfo;
    }

    VkDebugUtilsMessengerCreateInfoEXT make_info_debug_messenger(const PFN_vkDebugUtilsMessengerCallbackEXT& callback) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = callback_vk_debug;

        return createInfo;
    }

    bool check_validation_layer_support() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for ( const char* layer_name : ::VAL_LAYERS_TO_USE ) {
            bool layer_found = false;

            for ( const auto& layerProperties : availableLayers ) {
                if ( strcmp(layer_name, layerProperties.layerName) == 0 ) {
                    layer_found = true;
                    break;
                }
            }

            if ( !layer_found )
                return false;
        }

        return true;
    }


    VkInstance create_vulkan_instance(const char* const window_title, std::vector<const char*> extensions) {
        const auto appInfo = ::make_info_vulkan_app(window_title);
#ifdef DAL_VK_DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        const auto debug_info = ::make_info_debug_messenger(nullptr);
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef DAL_VK_DEBUG
        if (!::check_validation_layer_support()) {
            return nullptr;
        }

        createInfo.enabledLayerCount = ::VAL_LAYERS_TO_USE.size();
        createInfo.ppEnabledLayerNames = ::VAL_LAYERS_TO_USE.data();
        createInfo.pNext = reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debug_info);
#endif

        VkInstance instance = VK_NULL_HANDLE;
        const auto create_result = vkCreateInstance(&createInfo, nullptr, &instance);

        if (VK_SUCCESS == create_result) {
            return instance;
        }
        else {
            return VK_NULL_HANDLE;
        }
    }

}

namespace dal {

    class VulkanState::Pimpl {

    private:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    public:
        ~Pimpl() {
            this->destroy();
        }

        bool init(
            const char* const window_title,
            const std::vector<const char*>& extensions,
            const std::function<void*(void*)> surface_create_func
        ) {
#ifdef __ANDROID__
            if (0 == InitVulkan()) {
                return false;
            }
#endif
            this->destroy();

            this->m_instance = ::create_vulkan_instance(window_title, extensions);
            this->m_surface = reinterpret_cast<VkSurfaceKHR>(surface_create_func(this->m_instance));
            if (nullptr == this->m_surface) {
                return false;
            }

            const auto phys_devices = ::get_phys_devices(this->m_instance, this->m_surface);

            auto& logger = dal::LoggerSingleton::inst();
            const auto ss = std::string{} + "There are " + std::to_string(phys_devices.size()) + " physical devices: ";
            logger.simple_print(ss.c_str());
            for (auto& x : phys_devices) {
                std::string text;
                text += x.name();
                text += " (";
                text += x.device_type_str();
                text += ") : ";
                text += std::to_string(x.calc_score());
                logger.simple_print(text.c_str());
            }

            return this->is_ready();
        }

        void destroy() {
            if (VK_NULL_HANDLE != this->m_surface) {
                vkDestroySurfaceKHR(this->m_instance, this->m_surface, nullptr);
                this->m_surface = VK_NULL_HANDLE;
            }

            if (VK_NULL_HANDLE != this->m_instance) {
                vkDestroyInstance(this->m_instance, nullptr);
                this->m_instance = VK_NULL_HANDLE;
            }
        }

        bool is_ready() const {
            if (VK_NULL_HANDLE == this->m_instance) {
                return false;
            }
            if (VK_NULL_HANDLE == this->m_surface) {
                return false;
            }

            return true;
        }

    };

}


namespace dal {

    VulkanState::~VulkanState() {
        this->destroy();
    }

    bool VulkanState::init(
        const char* const window_title,
        const std::vector<const char*>& extensions,
        std::function<void*(void*)> surface_create_func
    ) {
        this->m_pimpl = new Pimpl;
        return this->m_pimpl->init(window_title, extensions, surface_create_func);
    }

    void VulkanState::destroy() {
        if (nullptr != this->m_pimpl) {
            delete this->m_pimpl;
            this->m_pimpl = nullptr;
        }
    }

}
