#include "d_vulkan_man.h"

#include <set>
#include <array>
#include <string>
#include <cstring>
#include <stdexcept>

#include <fmt/format.h>

#include "d_timer.h"
#include "d_logger.h"
#include "d_defines.h"
#include "d_vert_data.h"
#include "d_image_parser.h"


namespace {

    constexpr float PROJ_NEAR = 0.1;
    constexpr float PROJ_FAR = 50;


    VkExtent2D calc_smaller_extent(const VkExtent2D& extent, const float scale) {
        return VkExtent2D{
            std::max<uint32_t>(1, static_cast<uint32_t>(static_cast<float>(extent.width) * scale)),
            std::max<uint32_t>(1, static_cast<uint32_t>(static_cast<float>(extent.height) * scale))
        };
    }

    float calc_clip_depth(const float z, const glm::mat4& proj_mat) {
        const auto a = proj_mat * glm::vec4{0, 0, z, 1};
        const auto output = a.z / a.w;
        return output;
    }

    template <typename T>
    T calc_clip_depth(const T z, const T n, const T f) {
        dalAssert(static_cast<T>(0) != z);
        dalAssert(f != n);

        return (f * (z + n)) / (z * (f - n));
    }

    template <typename T>
    T calc_clip_depth_inv(const T depth, const T n, const T f) {
        dalAssert(static_cast<T>(0) != n || static_cast<T>(0) != f);

        return f*n / (depth*(f - n) - f);
    }


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

        for ( const char* layer_name : dal::VAL_LAYERS_TO_USE ) {
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
        const auto result_layer_support_check = ::check_validation_layer_support();
        dalAssert(result_layer_support_check);
        createInfo.enabledLayerCount = dal::VAL_LAYERS_TO_USE.size();
        createInfo.ppEnabledLayerNames = dal::VAL_LAYERS_TO_USE.data();

    #ifndef DAL_OS_ANDROID
        const auto debug_info = ::make_info_debug_messenger();
        createInfo.pNext = reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debug_info);
    #endif

#endif

        VkInstance instance = VK_NULL_HANDLE;
        const auto create_result = vkCreateInstance(&createInfo, nullptr, &instance);
        dalAssert(VK_SUCCESS == create_result);

        return instance;
    }

}


// VulkanState
namespace dal {

    VulkanState::VulkanState(
        const char* const window_title,
        const unsigned init_width,
        const unsigned init_height,
        const dal::RendererConfig& config,
        dal::Filesystem& filesys,
        dal::TaskManager& task_man,
        dal::ITextureManager& texture_man,
        const std::vector<const char*>& extensions,
        surface_create_func_t surface_create_func
    )
        : m_filesys(filesys)
        , m_texture_man(texture_man)
        , m_config(config)
        , m_new_extent(VkExtent2D{ init_width, init_height })
    {
#ifdef DAL_OS_ANDROID
        dalAssert(1 == InitVulkan());
#endif
        this->m_instance = ::create_vulkan_instance(window_title, extensions);

#ifdef DAL_SYS_X32
        this->m_surface = surface_create_func(this->m_instance);
#elif defined(DAL_SYS_X64)
        this->m_surface = reinterpret_cast<VkSurfaceKHR>(surface_create_func(this->m_instance));
#else
        #error Not supported system bit size
#endif
        dalAssert(VK_NULL_HANDLE != this->m_surface);

#ifdef DAL_VK_DEBUG
        this->m_debug_messenger = ::create_debug_messenger(this->m_instance);
        dalAssert(VK_NULL_HANDLE != this->m_debug_messenger);
#endif

        std::tie(this->m_phys_device, this->m_phys_info) = dal::get_best_phys_device(this->m_instance, this->m_surface, true);
        this->m_logi_device.init(this->m_surface, this->m_phys_device, this->m_phys_info);
        this->m_desc_layout_man.init(this->m_logi_device.get());

        this->m_sampler_man.init(
            this->m_phys_info.does_support_anisotropic_sampling(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        const auto result_init_swapchain = this->init_swapchain_and_dependers();
        dalAssert(result_init_swapchain);

        this->m_desc_allocator.init(64, 64, 64, 64, this->m_logi_device.get());
    }

    VulkanState::~VulkanState() {
        this->wait_idle();

        this->m_ref_planes.destroy(this->m_logi_device.get());
        this->m_shadow_maps.destroy(this->m_logi_device.get());
        this->m_desc_allocator.destroy(this->m_logi_device.get());
        this->m_sampler_man.destroy(this->m_logi_device.get());
        this->m_desc_man.destroy(this->m_logi_device.get());
        this->m_ubuf_man.destroy(this->m_logi_device.get());
        this->m_cmd_man.destroy(this->m_logi_device.get());
        this->m_pipelines.destroy(this->m_logi_device.get());
        this->m_fbuf_man.destroy(this->m_logi_device.get());
        this->m_renderpasses.destroy(this->m_logi_device.get());
        this->m_attach_man.destroy(this->m_logi_device.get());
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

    void VulkanState::update(const ICamera& camera, dal::Scene& scene) {
        if (this->m_screen_resize_notified) {
            this->m_screen_resize_notified = this->on_recreate_swapchain();
            return;
        }

        auto& sync_man = this->m_swapchain.sync_man();

        sync_man.m_fence_frame_in_flight.at(this->m_flight_frame_index).wait(this->m_logi_device.get());
        const auto [acquire_result, swapchain_index] = this->m_swapchain.acquire_next_img_index(this->m_flight_frame_index, this->m_logi_device.get());

        {
            if (ImgAcquireResult::out_of_date == acquire_result || ImgAcquireResult::suboptimal == acquire_result || this->m_screen_resize_notified) {
                this->m_screen_resize_notified = this->on_recreate_swapchain();
                return;
            }
            else if (ImgAcquireResult::success != acquire_result) {
                dalError("Failed to acquire swapchain image");
                return;
            }

            auto& img_fences = sync_man.fence_image_in_flight(swapchain_index);
            if (nullptr != img_fences) {
                img_fences->wait(this->m_logi_device.get());
            }
            img_fences = &sync_man.m_fence_frame_in_flight.at(this->m_flight_frame_index);
        }

        // Update render list
        //-----------------------------------------------------------------------------------------------------

        dal::RenderListVK render_list;
        render_list.apply(scene, camera.view_pos());

        for (auto x : render_list.m_used_actors) {
            auto& actor = dal::actor_cast(x);

            if (actor.m_transform_update_needed > 0) {
                --actor.m_transform_update_needed;
                actor.apply_transform(this->in_flight_index());
            }
        }

        for (auto x : render_list.m_used_skin_actors) {
            auto& actor = dal::actor_cast(x);

            actor.apply_animation(this->in_flight_index());
            actor.apply_transform(this->in_flight_index());
        }

        // Prepare needed data
        //-----------------------------------------------------------------------------------------------------

        const auto cur_sec = dal::get_cur_sec();

        const auto ref_mat = [&]() -> glm::mat4 {
            std::vector<glm::mat4> matrices;
            {
                const auto& mirror = scene.m_mirrors.back();

                glm::vec3 vert_sum{0};
                for (auto& x : mirror.m_vertices)
                    vert_sum += x;

                auto& plane = mirror.m_plane;
                const auto [r, t] = plane.make_origin_align_mat_r_t(vert_sum / static_cast<float>(mirror.m_vertices.size()));
                const auto f = dal::make_upside_down_mat();
                const auto a = r * t;
                const auto m = glm::inverse(a) * f * a;

                matrices.push_back(glm::mat4{1});
                matrices.push_back(t);
                matrices.push_back(r * t);
                matrices.push_back(dal::make_upside_down_mat() * r * t);
                matrices.push_back(glm::inverse(r) * dal::make_upside_down_mat() * r * t);
                matrices.push_back(glm::inverse(t) * glm::inverse(r) * dal::make_upside_down_mat() * r * t);
            }

            const auto normalized_range = cos(dal::get_cur_sec() * 0.2) * 0.5 + 0.5;
            const auto index_float = normalized_range * static_cast<double>(matrices.size() - 1);
            const auto index_int = static_cast<size_t>(floor(index_float));

            const auto index_from = index_int % matrices.size();
            const auto index_to = (index_from + 1) % matrices.size();
            // dalInfo(fmt::format("{}, {}, {}, {}, {}", normalized_range, index_float, index_int, index_from, index_to).c_str());
            const auto interval = static_cast<float>(index_float - static_cast<double>(index_int));

            return matrices.at(index_from) * (1.f - interval) + (matrices.at(index_to) * interval);
        }();

        const auto cam_view_mat = camera.make_view_mat() * ref_mat;
        const auto cam_proj_mat = make_perspective_proj_mat(glm::radians<float>(80), this->m_swapchain.perspective_ratio(), ::PROJ_NEAR, ::PROJ_FAR);
        const auto cam_proj_view_mat = cam_proj_mat * cam_view_mat;

        const auto dlight_update_flags = this->m_shadow_maps.create_dlight_update_flags();

        std::array<std::array<glm::vec3, 8>, dal::MAX_DLIGHT_COUNT> frustum_vertices;
        {
            constexpr float PROJ_DIST = ::PROJ_FAR - ::PROJ_NEAR;
            constexpr float NEAR_SCALAR = 0.2f;
            auto far_dist = PROJ_DIST;
            auto near_dist = far_dist * NEAR_SCALAR;

            for (size_t i = 0; i < frustum_vertices.size() - 1; ++i) {
                const auto index = frustum_vertices.size() - i - 1;

                if (dlight_update_flags[index]) {
                    frustum_vertices[index] = camera.make_frustum_vertices(
                        glm::radians<float>(80),
                        this->m_swapchain.perspective_ratio(),
                        ::PROJ_NEAR + near_dist,
                        ::PROJ_NEAR + far_dist
                    );

                    this->m_shadow_maps.m_dlight_clip_dists[index] = ::calc_clip_depth(-(::PROJ_NEAR + far_dist), ::PROJ_NEAR, ::PROJ_FAR);

                    this->m_shadow_maps.m_dlight_matrices[index] = render_list.m_dlight.make_light_mat(
                        frustum_vertices[index].data(), frustum_vertices[index].data() + frustum_vertices[index].size()
                    );
                }

                far_dist = near_dist;
                near_dist = far_dist * NEAR_SCALAR;
            }

            frustum_vertices[0] = camera.make_frustum_vertices(
                glm::radians<float>(80),
                this->m_swapchain.perspective_ratio(),
                ::PROJ_NEAR,
                ::PROJ_NEAR + far_dist
            );

            this->m_shadow_maps.m_dlight_clip_dists[0] = ::calc_clip_depth(-(::PROJ_NEAR + far_dist), ::PROJ_NEAR, ::PROJ_FAR);

            this->m_shadow_maps.m_dlight_matrices[0] = render_list.m_dlight.make_light_mat(
                frustum_vertices[0].data(), frustum_vertices[0].data() + frustum_vertices[0].size()
            );
        }

        // Set mirror plane
        {
            const auto extent_reflection = ::calc_smaller_extent(this->m_new_extent, 0.7);

            this->m_ref_planes.resize(
                render_list.m_render_planes.size() + render_list.m_render_waters.size(),
                extent_reflection.width, extent_reflection.height,
                this->m_desc_layout_man.layout_mirror(),
                this->m_renderpasses.rp_simple(),
                this->m_phys_device.get(),
                this->m_logi_device.get()
            );

            auto& dst_planes = this->m_ref_planes.reflection_planes();
            auto& render_planes = render_list.m_render_planes;
            auto& render_waters = render_list.m_render_waters;

            size_t index_counter = 0;

            for (auto& x : render_planes) {
                dst_planes[index_counter].m_orient_mat = x.m_orient_mat;
                dst_planes[index_counter].m_clip_plane = x.m_clip_plane;
                x.reflection_map_index = index_counter++;
            }

            for (auto& x : render_waters) {
                dst_planes[index_counter].m_orient_mat = x.m_orient_mat;
                dst_planes[index_counter].m_clip_plane = x.m_clip_plane;
                x.reflection_map_index = index_counter++;
            }
        }

        // Set up uniform variables
        //-----------------------------------------------------------------------------------------------------

        // U_CameraTransform
        {
            U_CameraTransform ubuf_data_composition{};
            ubuf_data_composition.m_view = cam_view_mat;
            ubuf_data_composition.m_proj = cam_proj_mat;
            ubuf_data_composition.m_view_inv = glm::inverse(cam_view_mat);
            ubuf_data_composition.m_proj_inv = glm::inverse(cam_proj_mat);
            ubuf_data_composition.m_view_pos = glm::vec4{ camera.view_pos(), 1 };
            ubuf_data_composition.m_near = ::PROJ_NEAR;
            ubuf_data_composition.m_far = ::PROJ_FAR;
            this->m_ubuf_man.m_u_camera_transform.at(this->m_flight_frame_index.get()).copy_to_buffer(ubuf_data_composition, this->m_logi_device.get());
        }

        // U_GlobalLight
        {
            dalAssert(render_list.m_plights.size() <= dal::MAX_PLIGHT_COUNT);
            dalAssert(render_list.m_slights.size() <= dal::MAX_SLIGHT_COUNT);

            U_GlobalLight data_glight{};

            data_glight.m_dlight_count = dal::MAX_DLIGHT_COUNT;
            for (size_t i = 0; i < dal::MAX_DLIGHT_COUNT; ++i) {
                auto& src_light = render_list.m_dlight;

                data_glight.m_dlight_mat[i]   = this->m_shadow_maps.m_dlight_matrices[i];
                data_glight.m_dlight_direc[i] = glm::vec4{ src_light.to_light_direc(), 0 };
                data_glight.m_dlight_color[i] = glm::vec4{ src_light.m_color, 1 };
                data_glight.m_dlight_clip_dist[i] = this->m_shadow_maps.m_dlight_clip_dists[i];
            }

            data_glight.m_plight_count = render_list.m_plights.size();
            for (size_t i = 0; i < render_list.m_plights.size(); ++i) {
                data_glight.m_plight_pos_n_max_dist[i] = glm::vec4{ render_list.m_plights[i].m_pos, 0 };
                data_glight.m_plight_color[i]          = glm::vec4{ render_list.m_plights[i].m_color, 0 };
            }

            data_glight.m_slight_count = render_list.m_slights.size();
            for (size_t i = 0; i < render_list.m_slights.size(); ++i) {
                auto& src_light = render_list.m_slights[i];

                data_glight.m_slight_mat[i]                = src_light.make_light_mat();
                data_glight.m_slight_pos_n_max_dist[i]     = glm::vec4{ src_light.m_pos, src_light.m_max_dist };
                data_glight.m_slight_direc_n_fade_start[i] = glm::vec4{ src_light.to_light_direc(), src_light.fade_start() };
                data_glight.m_slight_color_n_fade_end[i]   = glm::vec4{ src_light.m_color, src_light.fade_end() };
            }

            data_glight.m_ambient_light = glm::vec4{ render_list.m_ambient_light, 1 };
            data_glight.m_atmos_intensity = render_list.m_dlight.m_atmos_intensity;
            data_glight.m_mie_scattering_coeff = 221e-6;

            this->m_ubuf_man.m_ub_glights.at(this->m_flight_frame_index.get()).copy_to_buffer(data_glight, this->m_logi_device.get());
        }

        // Submit command buffers to GPU
        //-----------------------------------------------------------------------------------------------------

        // Reflection planes
        {
            std::array<VkPipelineStageFlags, 0> wait_stages{};
            std::array<VkSemaphore, 0> wait_semaphores{};
            std::array<VkSemaphore, 0> signal_semaphores{};

            for (auto& plane : this->m_ref_planes.reflection_planes()) {
                const auto& cmd_buf = plane.m_cmd_buf.at(this->m_flight_frame_index.get());

                U_PC_OnMirror pc_data;
                pc_data.m_proj_view_mat = cam_proj_view_mat * plane.m_orient_mat;
                pc_data.m_clip_plane = plane.m_clip_plane;

                record_cmd_on_mirror(
                    cmd_buf,
                    render_list,
                    this->m_flight_frame_index,
                    pc_data,
                    plane.m_attachments.extent(),
                    this->m_pipelines.on_mirror(),
                    this->m_pipelines.on_mirror_animated(),
                    plane.m_fbuf,
                    this->m_renderpasses.rp_simple()
                );

                VkSubmitInfo submit_info{};
                submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.pCommandBuffers = &cmd_buf;
                submit_info.commandBufferCount = 1;
                submit_info.waitSemaphoreCount = wait_semaphores.size();
                submit_info.pWaitSemaphores = wait_semaphores.data();
                submit_info.pWaitDstStageMask = wait_stages.data();
                submit_info.signalSemaphoreCount = signal_semaphores.size();
                submit_info.pSignalSemaphores = signal_semaphores.data();

                const auto submit_result = vkQueueSubmit(
                    this->m_logi_device.queue_graphics(),
                    1,
                    &submit_info,
                    VK_NULL_HANDLE
                );

                dalAssert(VK_SUCCESS == submit_result);
            }
        }

        // Shadow maps
        {
            std::array<VkPipelineStageFlags, 0> wait_stages{};
            std::array<VkSemaphore, 0> wait_semaphores{};
            std::array<VkSemaphore, 0> signal_semaphores{};

            for (size_t i = 0; i < dal::MAX_DLIGHT_COUNT; ++i) {
                if (!dlight_update_flags[i])
                    continue;

                auto& shadow_map = this->m_shadow_maps.m_dlights[i];

                record_cmd_shadow(
                    shadow_map.cmd_buf_at(this->m_flight_frame_index.get()),
                    render_list,
                    this->m_flight_frame_index,
                    this->m_shadow_maps.m_dlight_matrices[i],
                    shadow_map.extent(),
                    this->m_pipelines.shadow(),
                    this->m_pipelines.shadow_animated(),
                    shadow_map.fbuf(),
                    this->m_renderpasses.rp_shadow()
                );

                VkSubmitInfo submit_info{};
                submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.pCommandBuffers = &this->m_shadow_maps.m_dlights[i].cmd_buf_at(this->m_flight_frame_index.get());
                submit_info.commandBufferCount = 1;
                submit_info.waitSemaphoreCount = wait_semaphores.size();
                submit_info.pWaitSemaphores = wait_semaphores.data();
                submit_info.pWaitDstStageMask = wait_stages.data();
                submit_info.signalSemaphoreCount = signal_semaphores.size();
                submit_info.pSignalSemaphores = signal_semaphores.data();

                const auto submit_result = vkQueueSubmit(
                    this->m_logi_device.queue_graphics(),
                    1,
                    &submit_info,
                    VK_NULL_HANDLE
                );

                dalAssert(VK_SUCCESS == submit_result);
            }

            for (size_t i = 0; i < render_list.m_slights.size(); ++i) {
                auto& shadow_map = this->m_shadow_maps.m_slights[i];

                record_cmd_shadow(
                    shadow_map.cmd_buf_at(this->m_flight_frame_index.get()),
                    render_list,
                    this->m_flight_frame_index,
                    render_list.m_slights[i].make_light_mat(),
                    shadow_map.extent(),
                    this->m_pipelines.shadow(),
                    this->m_pipelines.shadow_animated(),
                    shadow_map.fbuf(),
                    this->m_renderpasses.rp_shadow()
                );

                VkSubmitInfo submit_info{};
                submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.pCommandBuffers = &shadow_map.cmd_buf_at(this->m_flight_frame_index.get());
                submit_info.commandBufferCount = 1;
                submit_info.waitSemaphoreCount = wait_semaphores.size();
                submit_info.pWaitSemaphores = wait_semaphores.data();
                submit_info.pWaitDstStageMask = wait_stages.data();
                submit_info.signalSemaphoreCount = signal_semaphores.size();
                submit_info.pSignalSemaphores = signal_semaphores.data();

                const auto submit_result = vkQueueSubmit(
                    this->m_logi_device.queue_graphics(),
                    1,
                    &submit_info,
                    VK_NULL_HANDLE
                );

                dalAssert(VK_SUCCESS == submit_result);
            }
        }

        // Gbuf
        {
            std::array<VkPipelineStageFlags, 1> wait_stages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            std::array<VkSemaphore, 1> wait_semaphores{ sync_man.m_semaph_img_available.at(this->m_flight_frame_index).get() };
            std::array<VkSemaphore, 1> signal_semaphores{ sync_man.m_semaph_cmd_done_gbuf.at(this->m_flight_frame_index).get() };

            record_cmd_gbuf(
                this->m_cmd_man.cmd_simple_at(this->m_flight_frame_index.get()),
                render_list,
                this->m_flight_frame_index,
                cam_proj_mat * cam_view_mat,
                this->m_ref_planes,
                this->m_attach_man.color().extent(),
                this->m_desc_man.desc_set_per_global_at(this->m_flight_frame_index.get()),
                this->m_desc_man.desc_set_composition_at(this->m_flight_frame_index.get()).get(),
                this->m_pipelines.gbuf(),
                this->m_pipelines.gbuf_animated(),
                this->m_pipelines.composition(),
                this->m_pipelines.mirror(),
                this->m_fbuf_man.fbuf_gbuf_at(swapchain_index),
                this->m_renderpasses.rp_gbuf()
            );

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pCommandBuffers = &this->m_cmd_man.cmd_simple_at(this->m_flight_frame_index.get());
            submit_info.commandBufferCount = 1;
            submit_info.waitSemaphoreCount = wait_semaphores.size();
            submit_info.pWaitSemaphores = wait_semaphores.data();
            submit_info.pWaitDstStageMask = wait_stages.data();
            submit_info.signalSemaphoreCount = signal_semaphores.size();
            submit_info.pSignalSemaphores = signal_semaphores.data();

            const auto submit_result = vkQueueSubmit(
                this->m_logi_device.queue_graphics(),
                1,
                &submit_info,
                VK_NULL_HANDLE
            );

            dalAssert(VK_SUCCESS == submit_result);
        }

        // Alpha
        {
            std::array<VkPipelineStageFlags, 1> wait_stages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            std::array<VkSemaphore, 1> wait_semaphores{ sync_man.m_semaph_cmd_done_gbuf.at(this->m_flight_frame_index).get() };
            std::array<VkSemaphore, 1> signal_semaphores{ sync_man.m_semaph_cmd_done_alpha.at(this->m_flight_frame_index).get() };

            record_cmd_alpha(
                this->m_cmd_man.cmd_alpha_at(this->m_flight_frame_index.get()),
                render_list,
                this->m_flight_frame_index,
                camera.view_pos(),
                this->m_attach_man.color().extent(),
                this->m_desc_man.desc_set_alpha_at(this->m_flight_frame_index.get()),
                this->m_desc_man.desc_set_composition_at(this->m_flight_frame_index.get()).get(),
                this->m_pipelines.alpha(),
                this->m_pipelines.alpha_animated(),
                this->m_fbuf_man.fbuf_alpha_at(swapchain_index),
                this->m_renderpasses.rp_alpha()
            );

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pCommandBuffers = &this->m_cmd_man.cmd_alpha_at(this->m_flight_frame_index.get());
            submit_info.commandBufferCount = 1;
            submit_info.waitSemaphoreCount = wait_semaphores.size();
            submit_info.pWaitSemaphores = wait_semaphores.data();
            submit_info.pWaitDstStageMask = wait_stages.data();
            submit_info.signalSemaphoreCount = signal_semaphores.size();
            submit_info.pSignalSemaphores = signal_semaphores.data();

            const auto submit_result = vkQueueSubmit(
                this->m_logi_device.queue_graphics(),
                1,
                &submit_info,
                VK_NULL_HANDLE
            );

            dalAssert(VK_SUCCESS == submit_result);
        }

        // Final
        {
            std::array<VkPipelineStageFlags, 1> wait_stages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            std::array<VkSemaphore, 1> wait_semaphores{ sync_man.m_semaph_cmd_done_alpha.at(this->m_flight_frame_index).get() };
            std::array<VkSemaphore, 1> signal_semaphores{ sync_man.m_semaph_cmd_done_final.at(this->m_flight_frame_index).get() };
            auto& fence = sync_man.m_fence_frame_in_flight.at(this->m_flight_frame_index);

            fence.wait_reset(this->m_logi_device.get());

            record_cmd_final(
                this->m_cmd_man.cmd_final_at(this->m_flight_frame_index.get()),
                this->m_swapchain.identity_extent(),
                this->m_desc_man.desc_set_final_at(this->m_flight_frame_index.get()),
                this->m_pipelines.final(),
                this->m_fbuf_man.fbuf_final_at(swapchain_index),
                this->m_renderpasses.rp_final()
            );

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pCommandBuffers = &this->m_cmd_man.cmd_final_at(this->m_flight_frame_index.get());
            submit_info.commandBufferCount = 1;
            submit_info.waitSemaphoreCount = wait_semaphores.size();
            submit_info.pWaitSemaphores = wait_semaphores.data();
            submit_info.pWaitDstStageMask = wait_stages.data();
            submit_info.signalSemaphoreCount = signal_semaphores.size();
            submit_info.pSignalSemaphores = signal_semaphores.data();

            const auto submit_result = vkQueueSubmit(
                this->m_logi_device.queue_graphics(),
                1,
                &submit_info,
                fence.get()
            );

            dalAssert(VK_SUCCESS == submit_result);
        }

        // Present
        {
            std::array<VkSemaphore, 1> wait_semaphores{ sync_man.m_semaph_cmd_done_final.at(this->m_flight_frame_index).get() };
            std::array<VkSwapchainKHR, 1> swapchains{ this->m_swapchain.swapchain() };

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = wait_semaphores.size();
            presentInfo.pWaitSemaphores    = wait_semaphores.data();
            presentInfo.swapchainCount     = swapchains.size();
            presentInfo.pSwapchains        = swapchains.data();
            presentInfo.pImageIndices      = &*swapchain_index;
            presentInfo.pResults           = nullptr;

            vkQueuePresentKHR(this->m_logi_device.queue_present(), &presentInfo);
        }

        // Finish one frame
        //-----------------------------------------------------------------------------------------------------

        this->m_flight_frame_index.increase();
        ++this->m_frame_count;
    }

    void VulkanState::wait_idle() {
        this->m_logi_device.wait_idle();
    }

    void VulkanState::on_screen_resize(const unsigned width, const unsigned height) {
        this->m_screen_resize_notified = true;
        this->m_new_extent.width = width;
        this->m_new_extent.height = height;

        dalVerbose(fmt::format("Screen resized: {} x {}", this->m_new_extent.width, this->m_new_extent.height).c_str());
    }

    void VulkanState::apply_config(const RendererConfig& config) {
        dalAbort("Not implemented");
    }

    HTexture VulkanState::create_texture() {
        return std::make_shared<TextureUnit>();
    }

    HRenModel VulkanState::create_model() {
        return std::make_shared<ModelRenderer>();
    }

    HRenModelSkinned VulkanState::create_model_skinned() {
        return std::make_shared<ModelSkinnedRenderer>();
    }

    HActor VulkanState::create_actor() {
        return std::make_shared<ActorVK>();
    }

    HActorSkinned VulkanState::create_actor_skinned() {
        return std::make_shared<ActorSkinnedVK>();
    }

    bool VulkanState::init(ITexture& h_tex, const ImageData& img_data) {
        auto& tex = reinterpret_cast<TextureUnit&>(h_tex);

        return tex.init(
            this->m_cmd_man.general_pool(),
            img_data,
            this->m_logi_device.queue_graphics(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );
    }

    bool VulkanState::init(IRenModel& h_model, const dal::ModelStatic& model_data, const char* const fallback_namespace) {
        auto& model = reinterpret_cast<ModelRenderer&>(h_model);

        model.init(this->m_phys_device.get(), this->m_logi_device.get());

        model.upload_meshes(
            model_data,
            this->m_cmd_man.general_pool(),
            this->m_texture_man,
            fallback_namespace,
            this->m_desc_layout_man.layout_per_actor(),
            this->m_desc_layout_man.layout_per_material(),
            this->m_logi_device.queue_graphics(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        return true;
    }

    bool VulkanState::init(IRenModelSkineed& model, const dal::ModelSkinned& model_data, const char* const fallback_namespace) {
        auto& m = reinterpret_cast<ModelSkinnedRenderer&>(model);

        m.init(this->m_phys_device.get(), this->m_logi_device.get());

        m.upload_meshes(
            model_data,
            this->m_cmd_man.general_pool(),
            this->m_texture_man,
            fallback_namespace,
            this->m_desc_layout_man.layout_per_actor(),
            this->m_desc_layout_man.layout_per_material(),
            this->m_logi_device.queue_graphics(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        return true;
    }

    bool VulkanState::init(IActor& actor) {
        auto& a = dal::actor_cast(actor);

        a.init(
            this->m_desc_allocator,
            this->m_desc_layout_man.layout_per_actor(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        return true;
    }

    bool VulkanState::init(IActorSkinned& actor) {
        auto& a = dal::actor_cast(actor);

        a.init(
            this->m_desc_allocator,
            this->m_desc_layout_man.layout_actor_animated(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        return true;
    }

    bool VulkanState::prepare(IRenModel& h_model) {
        auto& model = reinterpret_cast<ModelRenderer&>(h_model);

        return model.fetch_one_resource(
            this->m_desc_layout_man.layout_per_material(),
            this->m_sampler_man.sampler_tex(),
            this->m_logi_device.get()
        );
    }

    bool VulkanState::prepare(IRenModelSkineed& model) {
        auto& m = reinterpret_cast<ModelSkinnedRenderer&>(model);

        return m.fetch_one_resource(
            this->m_desc_layout_man.layout_per_material(),
            this->m_sampler_man.sampler_tex(),
            this->m_logi_device.get()
        );
    }

    // Mesh

    HMesh VulkanState::create_mesh() {
        return std::make_shared<MeshVK>();
    }

    bool VulkanState::init(IMesh& i_mesh, const std::vector<VertexStatic>& vertices, const std::vector<uint32_t>& indices) {
        auto& mesh = mesh_cast(i_mesh);

        mesh.init(
            vertices, indices,
            this->m_cmd_man.general_pool(),
            this->m_phys_device.get(),
            this->m_logi_device
        );

        return true;
    }

    bool VulkanState::destroy(IMesh& i_mesh) {
        auto& mesh = mesh_cast(i_mesh);

        mesh.destroy(this->m_logi_device.get());

        return true;
    }

    // Private
    //---------------------------------------------------------------------------------------

    bool VulkanState::on_recreate_swapchain() {
        if (0 == this->m_new_extent.width || 0 == this->m_new_extent.height) {
            return true;
        }
        this->wait_idle();

        return !this->init_swapchain_and_dependers();
    }

    [[nodiscard]]
    bool VulkanState::init_swapchain_and_dependers() {
        const auto result_swapchain = this->m_swapchain.init(
            this->m_new_extent.width,
            this->m_new_extent.height,
            this->m_logi_device.indices(),
            this->m_surface,
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        if (!result_swapchain)
            return false;

        this->m_renderpasses.init(
            this->m_swapchain.format(),
            this->m_phys_device.find_depth_format(),
            this->m_logi_device.get()
        );

        const auto extent_gbuf = ::calc_smaller_extent(this->m_new_extent, 0.9);

        this->m_attach_man.init(
            extent_gbuf,
            this->m_renderpasses.rp_gbuf(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        this->m_shadow_maps.init(this->m_renderpasses.rp_shadow(), this->m_phys_device, this->m_logi_device);

        this->m_ref_planes.init(
            this->m_phys_device.get(),
            this->m_logi_device
        );

        this->m_fbuf_man.init(
            this->m_swapchain.views(),
            this->m_attach_man,
            this->m_swapchain.identity_extent(),
            this->m_attach_man.color().extent(),
            this->m_renderpasses.rp_gbuf(),
            this->m_renderpasses.rp_final(),
            this->m_renderpasses.rp_alpha(),
            this->m_logi_device.get()
        );

        this->m_pipelines.init(
            this->m_filesys,
            this->m_config.m_shader,
            !this->m_swapchain.is_format_srgb(),
            this->m_phys_info.does_support_depth_clamp(),
            this->m_swapchain.identity_extent(),
            this->m_attach_man.color().extent(),
            this->m_desc_layout_man,
            this->m_renderpasses,
            this->m_logi_device.get()
        );

        this->m_cmd_man.init(
            MAX_FRAMES_IN_FLIGHT,
            this->m_logi_device.indices().graphics_family(),
            this->m_logi_device.get()
        );

        this->m_ubuf_man.init(this->m_phys_device.get(), this->m_logi_device.get());

        U_Shader_Final data;
        data.m_rotation = this->m_swapchain.pre_ratation_mat();
        this->m_ubuf_man.m_ub_final.copy_to_buffer(data, this->m_logi_device.get());

        this->m_desc_man.init(MAX_FRAMES_IN_FLIGHT, this->m_logi_device.get());

        for (int i = 0; i < dal::MAX_FRAMES_IN_FLIGHT; ++i) {
            auto& desc_per_global = this->m_desc_man.add_descset_per_global(
                this->m_desc_layout_man.layout_per_global(),
                this->m_logi_device.get()
            );

            desc_per_global.record_per_global(
                this->m_ubuf_man.m_u_camera_transform.at(i),
                this->m_ubuf_man.m_ub_glights.at(i),
                this->m_logi_device.get()
            );

            auto& desc_composition = this->m_desc_man.add_descset_composition(
                this->m_desc_layout_man.layout_composition(),
                this->m_logi_device.get()
            );

            const std::vector<VkImageView> color_attachments{
                this->m_attach_man.depth().view().get(),
                this->m_attach_man.albedo().view().get(),
                this->m_attach_man.materials().view().get(),
                this->m_attach_man.normal().view().get(),
            };

            desc_composition.record_composition(
                color_attachments,
                this->m_ubuf_man.m_ub_glights.at(i),
                this->m_ubuf_man.m_u_camera_transform.at(i),
                this->m_shadow_maps.dlight_views(),
                this->m_shadow_maps.slight_views(),
                this->m_sampler_man.sampler_depth(),
                this->m_logi_device.get()
            );

            auto& desc_final = this->m_desc_man.add_descset_final(
                this->m_desc_layout_man.layout_final(),
                this->m_logi_device.get()
            );

            desc_final.record_final(
                this->m_attach_man.color().view().get(),
                this->m_sampler_man.sampler_tex(),
                this->m_ubuf_man.m_ub_final,
                this->m_logi_device.get()
            );

            auto& desc_alpha = this->m_desc_man.add_descset_alpha(
                this->m_desc_layout_man.layout_alpha(),
                this->m_logi_device.get()
            );

            desc_alpha.record_alpha(
                this->m_ubuf_man.m_u_camera_transform.at(i),
                this->m_ubuf_man.m_ub_glights.at(i),
                this->m_shadow_maps.dlight_views(),
                this->m_shadow_maps.slight_views(),
                this->m_sampler_man.sampler_depth(),
                this->m_logi_device.get()
            );
        }

        this->m_shadow_maps.render_empty_for_all(
            this->m_pipelines,
            this->m_renderpasses.rp_shadow(),
            this->m_logi_device
        );

        return true;
    }

}
