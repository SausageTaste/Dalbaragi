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
#include "d_shader.h"
#include "d_render_pass.h"
#include "d_framebuffer.h"
#include "d_command.h"
#include "d_vert_data.h"
#include "d_uniform.h"


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
        dal::QueueFamilyIndices m_queue_indices;

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
                create_info_device.enabledExtensionCount   = ::PHYS_DEVICE_EXTENSIONS.size();
                create_info_device.ppEnabledExtensionNames = ::PHYS_DEVICE_EXTENSIONS.data();
#ifdef DAL_VK_DEBUG
                create_info_device.enabledLayerCount   = ::VAL_LAYERS_TO_USE.size();
                create_info_device.ppEnabledLayerNames = ::VAL_LAYERS_TO_USE.data();
#endif

                const auto create_result = vkCreateDevice(phys_device.get(), &create_info_device, nullptr, &this->m_handle);
                dalAssert(VK_SUCCESS == create_result);
            }

            vkGetDeviceQueue(this->m_handle, this->indices().graphics_family(), 0, &this->m_graphics_queue);
            vkGetDeviceQueue(this->m_handle, this->indices().present_family(), 0, &this->m_present_queue);
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

        const dal::QueueFamilyIndices& indices() const {
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
        PipelineManager m_pipelines;
        RenderPassManager m_renderpasses;
        FbufManager m_fbuf_man;
        CmdPoolManager m_cmd_man;
        DescSetLayoutManager m_desc_layout_man;
        UniformBufferArray<U_PerFrame> m_ubufs_simple;
        DescriptorManager m_desc_man;

        VertexBuffer m_vert_buf;

        // Non-vulkan members
        dal::filesystem::AssetManager& m_asset_man;

#ifdef DAL_VK_DEBUG
        VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
#endif

        size_t m_cur_frame_index = 0;
        bool m_screen_resize_notified = false;
        VkExtent2D m_new_extent;

    public:
        Pimpl(const Pimpl&) = delete;
        Pimpl& operator=(const Pimpl&) = delete;
        Pimpl(Pimpl&&) = delete;
        Pimpl& operator=(Pimpl&&) = delete;

    public:
        Pimpl(
            const char* const window_title,
            const unsigned init_width,
            const unsigned init_height,
            dal::filesystem::AssetManager& asset_mgr,
            const std::vector<const char*>& extensions,
            const std::function<void*(void*)> surface_create_func
        )
            : m_asset_man(asset_mgr)
        {
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

            this->m_logi_device.init(this->m_surface, this->m_phys_device, this->m_phys_info);

            this->m_desc_layout_man.init(this->m_logi_device.get());

            this->m_swapchain.init(
                init_width,
                init_height,
                this->m_logi_device.indices(),
                this->m_surface,
                this->m_phys_device.get(),
                this->m_logi_device.get()
            );
            dalInfo(fmt::format("Swapchain created: {} x {}", this->m_swapchain.width(), this->m_swapchain.height()).c_str());

            this->m_renderpasses.init({ this->m_swapchain.format() }, this->m_logi_device.get());

            this->m_fbuf_man.init(
                this->m_swapchain.views(),
                this->m_swapchain.extent(),
                this->m_renderpasses.rp_rendering().get(),
                this->m_logi_device.get()
            );

            this->m_pipelines.init(
                asset_mgr,
                this->m_swapchain.extent(),
                this->m_desc_layout_man.layout_simple(),
                this->m_renderpasses.rp_rendering().get(),
                this->m_logi_device.get()
            );

            this->m_cmd_man.init(
                this->m_swapchain.size(),
                this->m_logi_device.indices().graphics_family(),
                this->m_logi_device.get()
            );

            const std::vector<Vertex> vertices{
                {glm::vec3{-0.5, -0.5, 0}, glm::vec3{1.0, 1.0, 1.0}, glm::vec2{0, 0}},
                {glm::vec3{-0.5,  0.5, 0}, glm::vec3{1.0, 0.0, 0.0}, glm::vec2{0, 1}},
                {glm::vec3{ 0.5,  0.5, 0}, glm::vec3{0.0, 1.0, 0.0}, glm::vec2{1, 1}},
                {glm::vec3{ 0.5, -0.5, 0}, glm::vec3{0.0, 0.0, 1.0}, glm::vec2{1, 0}},
            };
            const std::vector<index_data_t> indices{
                0, 1, 2,
                0, 2, 3,
            };

            this->m_vert_buf.init(
                vertices, indices,
                this->m_cmd_man.pool_single_time(),
                this->m_logi_device.queue_graphics(),
                this->m_phys_device.get(),
                this->m_logi_device.get()
            );

            this->m_ubufs_simple.init(
                this->m_swapchain.size(),
                this->m_phys_device.get(),
                this->m_logi_device.get()
            );

            this->m_desc_man.init(this->m_swapchain.size(), this->m_logi_device.get());
            this->m_desc_man.init_desc_sets_simple(
                this->m_ubufs_simple,
                this->m_swapchain.size(),
                this->m_desc_layout_man.layout_simple(),
                this->m_logi_device.get()
            );

            this->m_cmd_man.record_all_simple(
                this->m_fbuf_man.swapchain_fbuf(),
                this->m_desc_man.desc_set_raw_simple(),
                this->m_swapchain.extent(),
                this->m_vert_buf.vertex_buffer(),
                this->m_vert_buf.index_buffer(),
                this->m_vert_buf.index_size(),
                this->m_pipelines.simple().layout(),
                this->m_pipelines.simple().pipeline(),
                this->m_renderpasses.rp_rendering().get()
            );
        }

        ~Pimpl() {
            this->destroy();
        }

        void destroy() {
            this->m_desc_man.destroy(this->m_logi_device.get());
            this->m_ubufs_simple.destroy(this->m_logi_device.get());
            this->m_vert_buf.destroy(this->m_logi_device.get());
            this->m_cmd_man.destroy(this->m_logi_device.get());
            this->m_pipelines.destroy(this->m_logi_device.get());
            this->m_fbuf_man.destroy(this->m_logi_device.get());
            this->m_renderpasses.destroy(this->m_logi_device.get());
            this->m_swapchain.destroy(this->m_logi_device.get());
            this->m_desc_layout_man.destroy(this->m_logi_device.get());
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

        void update() {
            if (this->m_screen_resize_notified) {
                this->m_screen_resize_notified = this->recreate_swapchain();
                return;
            }

            this->m_swapchain.sync_man().fence_frame_in_flight(this->m_cur_frame_index).wait(this->m_logi_device.get());
            const auto [acquire_result, img_index] = this->m_swapchain.acquire_next_img_index(this->m_cur_frame_index, this->m_logi_device.get());

            if (ImgAcquireResult::out_of_date == acquire_result || ImgAcquireResult::suboptimal == acquire_result || this->m_screen_resize_notified) {
                this->m_screen_resize_notified = this->recreate_swapchain();
                return;
            }
            else if (ImgAcquireResult::success == acquire_result) {

            }
            else {
                dalAbort("Failed to acquire swapchain image");
            }

            auto img_fences = this->m_swapchain.sync_man().fences_image_in_flight();
            if (nullptr != img_fences.at(img_index)) {
                img_fences.at(img_index)->wait(this->m_logi_device.get());
            }
            img_fences.at(img_index) = &this->m_swapchain.sync_man().fence_frame_in_flight(this->m_cur_frame_index);

            //-----------------------------------------------------------------------------------------------------

            U_PerFrame ubuf_data;
            ubuf_data.m_model = glm::translate(glm::mat4{1}, glm::vec3{0.2, 0.2, 0});
            this->m_ubufs_simple.at(img_index).copy_to_buffer(ubuf_data, this->m_logi_device.get());

            //-----------------------------------------------------------------------------------------------------

            std::array<VkSemaphore, 1> waitSemaphores{ this->m_swapchain.sync_man().semaphore_img_available(this->m_cur_frame_index).get() };
            std::array<VkPipelineStageFlags, 1> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            std::array<VkSemaphore, 1> signalSemaphores{ this->m_swapchain.sync_man().semaphore_render_finished(this->m_cur_frame_index).get() };

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = waitSemaphores.size();
            submitInfo.pWaitSemaphores = waitSemaphores.data();
            submitInfo.pWaitDstStageMask = waitStages.data();
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &this->m_cmd_man.cmd_buffer_at(img_index);
            submitInfo.signalSemaphoreCount = signalSemaphores.size();
            submitInfo.pSignalSemaphores = signalSemaphores.data();

            this->m_swapchain.sync_man().fence_frame_in_flight(this->m_cur_frame_index).reset(this->m_logi_device.get());
            if (vkQueueSubmit(this->m_logi_device.queue_graphics(), 1, &submitInfo, this->m_swapchain.sync_man().fence_frame_in_flight(this->m_cur_frame_index).get()) != VK_SUCCESS) {
                dalAbort("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = signalSemaphores.size();
            presentInfo.pWaitSemaphores = signalSemaphores.data();

            VkSwapchainKHR swapChains[] = {this->m_swapchain.swapchain()};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &img_index;
            presentInfo.pResults = nullptr;

            vkQueuePresentKHR(this->m_logi_device.queue_present(), &presentInfo);

            this->m_cur_frame_index = (this->m_cur_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        void wait_device_idle() const {
            vkDeviceWaitIdle(this->m_logi_device.get());
        }

        bool recreate_swapchain() {
            if (0 == this->m_new_extent.width || 0 == this->m_new_extent.height) {
                return true;
            }
            this->wait_device_idle();

            this->m_swapchain.init(
                this->m_new_extent.width,
                this->m_new_extent.height,
                this->m_logi_device.indices(),
                this->m_surface,
                this->m_phys_device.get(),
                this->m_logi_device.get()
            );

            this->m_renderpasses.init({ this->m_swapchain.format() }, this->m_logi_device.get());

            this->m_fbuf_man.init(
                this->m_swapchain.views(),
                this->m_swapchain.extent(),
                this->m_renderpasses.rp_rendering().get(),
                this->m_logi_device.get()
            );

            this->m_pipelines.init(
                this->m_asset_man,
                this->m_swapchain.extent(),
                this->m_desc_layout_man.layout_simple(),
                this->m_renderpasses.rp_rendering().get(),
                this->m_logi_device.get()
            );

            this->m_cmd_man.init(
                this->m_swapchain.size(),
                this->m_logi_device.indices().graphics_family(),
                this->m_logi_device.get()
            );

            this->m_ubufs_simple.init(
                this->m_swapchain.size(),
                this->m_phys_device.get(),
                this->m_logi_device.get()
            );

            this->m_desc_man.init(this->m_swapchain.size(), this->m_logi_device.get());
            this->m_desc_man.init_desc_sets_simple(
                this->m_ubufs_simple,
                this->m_swapchain.size(),
                this->m_desc_layout_man.layout_simple(),
                this->m_logi_device.get()
            );

            this->m_cmd_man.record_all_simple(
                this->m_fbuf_man.swapchain_fbuf(),
                this->m_desc_man.desc_set_raw_simple(),
                this->m_swapchain.extent(),
                this->m_vert_buf.vertex_buffer(),
                this->m_vert_buf.index_buffer(),
                this->m_vert_buf.index_size(),
                this->m_pipelines.simple().layout(),
                this->m_pipelines.simple().pipeline(),
                this->m_renderpasses.rp_rendering().get()
            );

            dalInfo(fmt::format("Swapchain recreated: {} x {}", this->m_swapchain.width(), this->m_swapchain.height()).c_str());
            return false;
        }

        void on_screen_resize(const unsigned width, const unsigned height) {
            this->m_screen_resize_notified = true;
            this->m_new_extent.width = width;
            this->m_new_extent.height = height;

            dalInfo(fmt::format("Screen resized: {} x {}", this->m_new_extent.width, this->m_new_extent.height).c_str());
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
        dal::filesystem::AssetManager& asset_mgr,
        const std::vector<const char*>& extensions,
        std::function<void*(void*)> surface_create_func
    ) {
        this->destroy();
        this->m_pimpl = new Pimpl(window_title, init_width, init_height, asset_mgr, extensions, surface_create_func);
        dalInfo(fmt::format("Init surface size: {} x {}", init_width, init_height).c_str());
    }

    void VulkanState::destroy() {
        if (nullptr != this->m_pimpl) {
            delete this->m_pimpl;
            this->m_pimpl = nullptr;
        }
    }

    void VulkanState::update() {
        this->m_pimpl->update();
    }

    void VulkanState::wait_device_idle() const {
        this->m_pimpl->wait_device_idle();
    }

    void VulkanState::on_screen_resize(const unsigned width, const unsigned height) {
        this->m_pimpl->on_screen_resize(width, height);
    }

}
