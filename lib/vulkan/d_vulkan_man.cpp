#include "d_vulkan_man.h"

#include <set>
#include <array>
#include <string>
#include <cstring>
#include <stdexcept>

#include <fmt/format.h>

#include "d_logger.h"

#include "d_vulkan_header.h"
#include "d_swapchain.h"


#if !defined(NDEBUG) && !defined(__ANDROID__)
    #define DAL_VK_DEBUG
#endif


// Vulkan utils
namespace {

    constexpr std::array<const char*, 1> VAL_LAYERS_TO_USE = {
        "VK_LAYER_KHRONOS_validation",
    };

    constexpr std::array<const char*, 1> PHYS_DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

}


// Physical device
namespace {

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
        PhysDeviceInfo(const VkSurfaceKHR surface, const VkPhysicalDevice phys_device) {
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

        bool does_support_anisotropic_sampling() const {
            return this->m_features.samplerAnisotropy;
        }

        bool is_usable() const {
            if (!this->does_support_all_extensions( ::PHYS_DEVICE_EXTENSIONS.begin(), ::PHYS_DEVICE_EXTENSIONS.end() ))
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


    class PhysicalDevice {

    private:

    private:
        VkPhysicalDevice m_handle = VK_NULL_HANDLE;

    public:
        PhysicalDevice() = default;
        PhysicalDevice(const PhysicalDevice&) = default;
        PhysicalDevice& operator=(const PhysicalDevice&) = default;

    public:
        PhysicalDevice(const VkPhysicalDevice phys_device)
            : m_handle(phys_device)
        {

        }

        auto& get() const {
            dalAssert(VK_NULL_HANDLE != this->m_handle);
            return this->m_handle;
        }

        auto make_info(const VkSurfaceKHR surface) const {
            return ::PhysDeviceInfo{ surface, this->m_handle };
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
            result.push_back(::PhysicalDevice{x});
        }

        return result;
    }

    template <bool _PrintInfo>
    auto get_best_phys_device(const VkInstance instance, const VkSurfaceKHR surface) {
        const auto phys_devices = ::get_phys_devices(instance);

        unsigned best_score = 0;
        PhysicalDevice best_device;
        PhysDeviceInfo best_info;

        if constexpr (_PrintInfo) {
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

            if constexpr (_PrintInfo) {
                dalInfo(fmt::format(" * {} ({}) : {}", info.name(), info.device_type_str(), this_score).c_str());
            }
        }

        return std::make_pair(best_device, best_info);
    }

}


// Logical device
namespace {

    class LogicalDevice {

    private:
        VkDevice m_handle = VK_NULL_HANDLE;

        VkQueue m_graphics_queue = VK_NULL_HANDLE;
        VkQueue m_present_queue = VK_NULL_HANDLE;

    public:
        LogicalDevice() = default;
        LogicalDevice(const LogicalDevice&) = delete;
        LogicalDevice& operator=(const LogicalDevice&) = delete;

    public:
        LogicalDevice(LogicalDevice&& other) noexcept {
            std::swap(this->m_handle, other.m_handle);
            std::swap(this->m_graphics_queue, other.m_graphics_queue);
            std::swap(this->m_present_queue, other.m_present_queue);
        }
        LogicalDevice& operator=(LogicalDevice&& other) noexcept {
            std::swap(this->m_handle, other.m_handle);
            std::swap(this->m_graphics_queue, other.m_graphics_queue);
            std::swap(this->m_present_queue, other.m_present_queue);
            return *this;
        }

        ~LogicalDevice() {
            this->destroy();
        }

        void init(const VkSurfaceKHR surface, const PhysicalDevice& phys_device, const PhysDeviceInfo& phys_info) {
            const dal::QueueFamilyIndices indices{ surface, phys_device.get() };

            // Create vulkan device
            {
                std::vector<VkDeviceQueueCreateInfo> create_info_queues;
                std::set<uint32_t> unique_queue_families{ indices.graphics_family(), indices.present_family() };
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
                create_info_device.enabledExtensionCount   = ::PHYS_DEVICE_EXTENSIONS.size();
                create_info_device.ppEnabledExtensionNames = ::PHYS_DEVICE_EXTENSIONS.data();
#ifdef DAL_VK_DEBUG
                create_info_device.enabledLayerCount   = ::VAL_LAYERS_TO_USE.size();
                create_info_device.ppEnabledLayerNames = ::VAL_LAYERS_TO_USE.data();
#endif

                const auto create_result = vkCreateDevice(phys_device.get(), &create_info_device, nullptr, &this->m_handle);
                dalAssert(VK_SUCCESS == create_result);
            }

            vkGetDeviceQueue(this->m_handle, indices.graphics_family(), 0, &this->m_graphics_queue);
            vkGetDeviceQueue(this->m_handle, indices.present_family(), 0, &this->m_present_queue);
        }

        void destroy() {
            // Queues are destoryed implicitly when the corresponding VkDevice is destroyed.

            if (VK_NULL_HANDLE != this->m_handle) {
                vkDestroyDevice(this->m_handle, nullptr);
                this->m_handle = VK_NULL_HANDLE;
            }
        }

        auto& get() const {
            return this->m_handle;
        }
        auto& queue_graphics() const {
            return this->m_graphics_queue;
        }
        auto& queue_present() const {
            return this->m_present_queue;
        }

    };

}


// Instance creation, validation layer
namespace {


#ifdef DAL_VK_DEBUG

    VKAPI_ATTR VkBool32 VKAPI_CALL callback_vk_debug(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        const auto err_msg = fmt::format("{}", pCallbackData->pMessage);

        switch ( messageSeverity ) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                dalVerbose(err_msg.c_str());
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                dalInfo(err_msg.c_str());
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                dalWarn(err_msg.c_str());
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                dalError(err_msg.c_str());
                break;
            default:
                dalWarn(err_msg.c_str())
                break;
        }

        return VK_FALSE;
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

    VkDebugUtilsMessengerCreateInfoEXT make_info_debug_messenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = callback_vk_debug;

        return createInfo;
    }

    void destroyDebugUtilsMessengerEXT(
        const VkInstance instance,
        const VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* const pAllocator
    ) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if ( func != nullptr ) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    VkResult createDebugUtilsMessengerEXT(
        const VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo,
        const VkAllocationCallbacks* const pAllocator,
        VkDebugUtilsMessengerEXT* const pDebugMessenger
    ) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (nullptr != func) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    VkDebugUtilsMessengerEXT create_debug_messenger(const VkInstance& instance) {
        const auto createInfo = ::make_info_debug_messenger();

        VkDebugUtilsMessengerEXT result = VK_NULL_HANDLE;
        if (VK_SUCCESS != ::createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &result)) {
            return VK_NULL_HANDLE;
        }

        return result;
    }

    void test_vk_validation(const VkPhysicalDevice phys_device) {
        VkDeviceCreateInfo info{};
        VkDevice tmp = VK_NULL_HANDLE;
        vkCreateDevice(phys_device, &info, nullptr, &tmp);
    }

#endif


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

    VkInstance create_vulkan_instance(const char* const window_title, std::vector<const char*> extensions) {
        const auto appInfo = ::make_info_vulkan_app(window_title);
#ifdef DAL_VK_DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef DAL_VK_DEBUG
        if (!::check_validation_layer_support()) {
            return nullptr;
        }

        const auto debug_info = ::make_info_debug_messenger();
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
        PhysicalDevice m_phys_device;
        PhysDeviceInfo m_phys_info;
        LogicalDevice m_logi_device;

        SwapchainManager m_swapchain;

#ifdef DAL_VK_DEBUG
        VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
#endif

    public:
        ~Pimpl() {
            this->destroy();
        }

        void init(
            const char* const window_title,
            const unsigned init_width,
            const unsigned init_height,
            const std::vector<const char*>& extensions,
            const std::function<void*(void*)> surface_create_func
        ) {
#ifdef __ANDROID__
            dalAssert(1 == InitVulkan());
#endif
            this->destroy();

            this->m_instance = ::create_vulkan_instance(window_title, extensions);
            dalAssert(VK_NULL_HANDLE != this->m_instance);

            this->m_surface = reinterpret_cast<VkSurfaceKHR>(surface_create_func(this->m_instance));
            dalAssert(VK_NULL_HANDLE != this->m_surface);

#ifdef DAL_VK_DEBUG
            this->m_debug_messenger = ::create_debug_messenger(this->m_instance);
            dalAssert(VK_NULL_HANDLE != this->m_debug_messenger);
#endif

            std::tie(this->m_phys_device, this->m_phys_info) = ::get_best_phys_device<true>(this->m_instance, this->m_surface);
#ifdef DAL_VK_DEBUG
            //::test_vk_validation(phys_devices[0].get());
#endif

            this->m_logi_device.init(this->m_surface, this->m_phys_device, this->m_phys_info);
            this->m_swapchain.init(init_width, init_height, this->m_surface, this->m_phys_device.get(), this->m_logi_device.get());
        }

        void destroy() {
            this->m_swapchain.destroy(this->m_logi_device.get());
            this->m_logi_device.destroy();

#ifdef DAL_VK_DEBUG
            if (VK_NULL_HANDLE != this->m_debug_messenger) {
                ::destroyDebugUtilsMessengerEXT(this->m_instance, this->m_debug_messenger, nullptr);
                this->m_debug_messenger = VK_NULL_HANDLE;
            }
#endif

            if (VK_NULL_HANDLE != this->m_surface) {
                vkDestroySurfaceKHR(this->m_instance, this->m_surface, nullptr);
                this->m_surface = VK_NULL_HANDLE;
            }

            if (VK_NULL_HANDLE != this->m_instance) {
                vkDestroyInstance(this->m_instance, nullptr);
                this->m_instance = VK_NULL_HANDLE;
            }
        }

    };

}


namespace dal {

    VulkanState::~VulkanState() {
        this->destroy();
    }

    void VulkanState::init(
        const char* const window_title,
        const unsigned init_width,
        const unsigned init_height,
        const std::vector<const char*>& extensions,
        std::function<void*(void*)> surface_create_func
    ) {
        this->m_pimpl = new Pimpl;
        this->m_pimpl->init(window_title, init_width, init_height, extensions, surface_create_func);
    }

    void VulkanState::destroy() {
        if (nullptr != this->m_pimpl) {
            delete this->m_pimpl;
            this->m_pimpl = nullptr;
        }
    }

}
