#include "d_vulkan_man.h"

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

    class PhysicalDevice {

    private:
        VkPhysicalDevice m_handle = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties m_properties;
        VkPhysicalDeviceFeatures m_features;

    public:
        PhysicalDevice() = default;

        PhysicalDevice(const VkPhysicalDevice phys_device)
            : m_handle(phys_device)
        {
            vkGetPhysicalDeviceProperties(this->m_handle, &this->m_properties);
            vkGetPhysicalDeviceFeatures(this->m_handle, &this->m_features);
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

    };


    std::vector<PhysicalDevice> get_phys_devices(const VkInstance instance) {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        std::vector<PhysicalDevice> result;
        result.reserve(device_count);
        for (const auto& x : devices) {
            result.emplace_back(x);
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

    public:
        ~Pimpl() {
            this->destroy();
        }

        bool init(const char* const window_title, std::vector<const char*> extensions) {
#ifdef __ANDROID__
            if (0 == InitVulkan()) {
                return false;
            }
#endif

            this->m_instance = ::create_vulkan_instance(window_title, extensions);

            const auto phys_devices = ::get_phys_devices(this->m_instance);

            auto& logger = dal::LoggerSingleton::inst();
            const auto ss = std::string{} + "There are " + std::to_string(phys_devices.size()) + " physical devices: ";
            logger.simple_print(ss.c_str());
            for (auto& x : phys_devices) {
                std::string text;
                text += x.name();
                text += " (";
                text += x.device_type_str();
                text += ")";
                logger.simple_print(text.c_str());
            }

            return this->is_ready();
        }

        void destroy() {
            if (VK_NULL_HANDLE != this->m_instance) {
                vkDestroyInstance(this->m_instance, nullptr);
                this->m_instance = VK_NULL_HANDLE;
            }
        }

        bool is_ready() const {
            if (VK_NULL_HANDLE == this->m_instance) {
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

    bool VulkanState::init(const char* const window_title, std::vector<const char*> extensions) {
        this->m_pimpl = new Pimpl;
        return this->m_pimpl->init(window_title, extensions);
    }

    void VulkanState::destroy() {
        if (nullptr != this->m_pimpl) {
            delete this->m_pimpl;
            this->m_pimpl = nullptr;
        }
    }

}
