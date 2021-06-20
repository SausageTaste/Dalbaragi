#include "d_vulkan_man.h"

#include <set>
#include <array>
#include <string>
#include <cstring>
#include <stdexcept>

#include <fmt/format.h>

#include "d_timer.h"
#include "d_logger.h"
#include "d_vert_data.h"
#include "d_image_parser.h"
#include "d_model_parser.h"


namespace {

    constexpr float DLIGHT_HALF_BOX_SIZE = 30;

    glm::mat4 make_perspective_proj_mat(const float ratio, const float fov) {
        auto mat = glm::perspective<float>(glm::radians(fov), ratio, 0.1f, 100.0f);
        mat[1][1] *= -1;
        return mat;
    }

    VkExtent2D calc_smaller_extent(const VkExtent2D& extent, const float scale) {
        return VkExtent2D{
            std::max<uint32_t>(1, static_cast<uint32_t>(static_cast<float>(extent.width) * scale)),
            std::max<uint32_t>(1, static_cast<uint32_t>(static_cast<float>(extent.height) * scale))
        };
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

    auto create_info_viewport_scissor(const VkExtent2D& extent) {
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0;
        viewport.maxDepth = 1;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        return std::make_pair(viewport, scissor);
    }

}


namespace dal {

    void ShadowMapFbuf::record_cmd_buf(
        const FrameInFlightIndex& flight_frame_index,
        const RenderList& render_list,
        const glm::mat4& light_mat,
        const ShaderPipeline& pipeline_shadow,
        const RenderPass_ShadowMap& render_pass
    ) {
        auto& cmd_buf = this->m_cmd_buf.at(flight_frame_index.get());

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if (VK_SUCCESS != vkBeginCommandBuffer(cmd_buf, &begin_info))
            dalAbort("failed to begin recording command buffer!");

        std::array<VkClearValue, 5> clear_colors{};
        clear_colors[0].depthStencil = { 1, 0 };

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass.get();
        render_pass_info.framebuffer = this->m_fbuf.get();
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = this->m_depth_attach.extent();
        render_pass_info.clearValueCount = clear_colors.size();
        render_pass_info.pClearValues = clear_colors.data();

        vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        {
            auto& pipeline = pipeline_shadow;

            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            const auto [viewport, scissor] = ::create_info_viewport_scissor(this->m_depth_attach.extent());
            vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
            vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& render_tuple : render_list.m_static_models) {
                auto& model = *reinterpret_cast<ModelRenderer*>(render_tuple.m_model.get());

                for (auto& unit : model.render_units()) {
                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    for (auto& actor : render_tuple.m_actors) {
                        U_PC_Shadow pc_data;
                        pc_data.m_model_mat = actor->m_transform.make_mat4();
                        pc_data.m_light_mat = light_mat;

                        vkCmdPushConstants(cmd_buf, pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(U_PC_Shadow), &pc_data);
                        vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                    }
                }
            }
        }

        vkCmdEndRenderPass(cmd_buf);

        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS)
            dalAbort("failed to record command buffer!");
    }

}


// VulkanState
namespace dal {

    VulkanState::VulkanState(
        const char* const window_title,
        const unsigned init_width,
        const unsigned init_height,
        dal::Filesystem& filesys,
        dal::TaskManager& task_man,
        dal::ITextureManager& texture_man,
        const std::vector<const char*>& extensions,
        std::function<void*(void*)> surface_create_func
    )
        : m_filesys(filesys)
        , m_texture_man(texture_man)
        , m_new_extent(VkExtent2D{ init_width, init_height })
    {
#ifdef DAL_OS_ANDROID
        dalAssert(1 == InitVulkan());
#endif
        this->m_instance = ::create_vulkan_instance(window_title, extensions);
        this->m_surface = reinterpret_cast<VkSurfaceKHR>(surface_create_func(this->m_instance));
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

        for (auto& x : this->m_dlight_shadow_maps)
            x.destroy(this->m_logi_device.get());

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

    void VulkanState::update(const ICamera& camera, const RenderList& render_list) {
        if (this->m_screen_resize_notified) {
            this->m_screen_resize_notified = this->on_recreate_swapchain();
            return;
        }

        auto& sync_man = this->m_swapchain.sync_man();

        sync_man.m_fence_frame_in_flight.at(this->m_flight_frame_index).wait(this->m_logi_device.get());
        const auto [acquire_result, swapchain_index] = this->m_swapchain.acquire_next_img_index(this->m_flight_frame_index, this->m_logi_device.get());

        if (ImgAcquireResult::out_of_date == acquire_result || ImgAcquireResult::suboptimal == acquire_result || this->m_screen_resize_notified) {
            this->m_screen_resize_notified = this->on_recreate_swapchain();
            return;
        }
        else if (ImgAcquireResult::success != acquire_result) {
            dalError("Failed to acquire swapchain image");
            return;
        }

        {
            auto& img_fences = sync_man.fence_image_in_flight(swapchain_index);
            if (nullptr != img_fences) {
                img_fences->wait(this->m_logi_device.get());
            }
            img_fences = &sync_man.m_fence_frame_in_flight.at(this->m_flight_frame_index);
        }

        //-----------------------------------------------------------------------------------------------------

        const auto cur_sec = dal::get_cur_sec();

        {
            U_PerFrame ubuf_data_per_frame{};
            ubuf_data_per_frame.m_view = camera.make_view_mat();
            ubuf_data_per_frame.m_proj = ::make_perspective_proj_mat(this->m_swapchain.perspective_ratio(), 80);
            ubuf_data_per_frame.m_view_pos = glm::vec4{ camera.view_pos(), 1 };
            this->m_ubuf_man.m_ub_simple.at(this->m_flight_frame_index.get()).copy_to_buffer(ubuf_data_per_frame, this->m_logi_device.get());

            U_PerFrame_Composition ubuf_data_composition{};
            ubuf_data_composition.m_proj_inv = glm::inverse(ubuf_data_per_frame.m_proj);
            ubuf_data_composition.m_view_inv = glm::inverse(ubuf_data_per_frame.m_view);
            ubuf_data_composition.m_view_pos = ubuf_data_per_frame.m_view_pos;
            this->m_ubuf_man.m_ub_per_frame_composition.at(this->m_flight_frame_index.get()).copy_to_buffer(ubuf_data_composition, this->m_logi_device.get());
        }

        {
            dalAssert(render_list.m_dlights.size() <= dal::MAX_DLIGHT_COUNT);
            dalAssert(render_list.m_plights.size() <= dal::MAX_PLIGHT_COUNT);

            U_GlobalLight data_glight{};
            {
                data_glight.m_dlight_count = render_list.m_dlights.size();
                for (size_t i = 0; i < render_list.m_dlights.size(); ++i) {
                    data_glight.m_dlight_mat[i]   = render_list.m_dlights[i].make_light_mat(::DLIGHT_HALF_BOX_SIZE);
                    data_glight.m_dlight_direc[i] = glm::vec4{ render_list.m_dlights[i].to_light_direc(), 0 };
                    data_glight.m_dlight_color[i] = glm::vec4{ render_list.m_dlights[i].m_color, 1 };
                }

                data_glight.m_plight_count = render_list.m_plights.size();
                for (size_t i = 0; i < render_list.m_plights.size(); ++i) {
                    data_glight.m_plight_pos_n_max_dist[i] = glm::vec4{ render_list.m_plights[i].m_pos, 0 };
                    data_glight.m_plight_color[i]          = glm::vec4{ render_list.m_plights[i].m_color, 0 };
                }

                data_glight.m_ambient_light = glm::vec4{ render_list.m_ambient_color, 1 };
            }

            this->m_ubuf_man.m_ub_glights.at(this->m_flight_frame_index.get()).copy_to_buffer(data_glight, this->m_logi_device.get());
        }

        //-----------------------------------------------------------------------------------------------------

        {
            std::array<VkPipelineStageFlags, 0> wait_stages{};
            std::array<VkSemaphore, 0> wait_semaphores{};
            std::array<VkSemaphore, 0> signal_semaphores{};

            for (size_t i = 0; i < dal::MAX_DLIGHT_COUNT; ++i) {
                this->m_dlight_shadow_maps[i].record_cmd_buf(
                    this->m_flight_frame_index,
                    render_list,
                    render_list.m_dlights[i].make_light_mat(::DLIGHT_HALF_BOX_SIZE),
                    this->m_pipelines.shadow(),
                    this->m_renderpasses.rp_shadow()
                );

                VkSubmitInfo submit_info{};
                submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.pCommandBuffers = &this->m_dlight_shadow_maps[i].cmd_buf_at(this->m_flight_frame_index.get());
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

        {
            std::array<VkPipelineStageFlags, 1> wait_stages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            std::array<VkSemaphore, 1> wait_semaphores{ sync_man.m_semaph_img_available.at(this->m_flight_frame_index).get() };
            std::array<VkSemaphore, 1> signal_semaphores{ sync_man.m_semaph_cmd_done_gbuf.at(this->m_flight_frame_index).get() };

            this->m_cmd_man.record_simple(
                this->m_flight_frame_index,
                render_list,
                this->m_desc_man.desc_set_per_global_at(this->m_flight_frame_index.get()),
                this->m_desc_man.desc_set_composition_at(this->m_flight_frame_index.get()).get(),
                this->m_attach_man.color().extent(),
                this->m_fbuf_man.swapchain_fbuf().at(swapchain_index.get()),
                this->m_pipelines.gbuf(),
                this->m_pipelines.gbuf_animated(),
                this->m_pipelines.composition(),
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

        {
            std::array<VkPipelineStageFlags, 1> wait_stages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            std::array<VkSemaphore, 1> wait_semaphores{ sync_man.m_semaph_cmd_done_gbuf.at(this->m_flight_frame_index).get() };
            std::array<VkSemaphore, 1> signal_semaphores{ sync_man.m_semaph_cmd_done_alpha.at(this->m_flight_frame_index).get() };

            this->m_cmd_man.record_alpha(
                this->m_flight_frame_index,
                camera.view_pos(),
                render_list,
                this->m_desc_man.desc_set_per_global_at(this->m_flight_frame_index.get()),
                this->m_desc_man.desc_set_composition_at(this->m_flight_frame_index.get()).get(),
                this->m_attach_man.color().extent(),
                this->m_fbuf_man.fbuf_alpha_at(swapchain_index).get(),
                this->m_pipelines.alpha(),
                this->m_pipelines.alpha_animated(),
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

        {
            std::array<VkPipelineStageFlags, 1> wait_stages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            std::array<VkSemaphore, 1> wait_semaphores{ sync_man.m_semaph_cmd_done_alpha.at(this->m_flight_frame_index).get() };
            std::array<VkSemaphore, 1> signal_semaphores{ sync_man.m_semaph_cmd_done_final.at(this->m_flight_frame_index).get() };
            auto& fence = sync_man.m_fence_frame_in_flight.at(this->m_flight_frame_index);

            fence.wait_reset(this->m_logi_device.get());

            this->m_desc_man.init_desc_sets_final(
                this->m_flight_frame_index.get(),
                this->m_ubuf_man.m_ub_final,
                this->m_attach_man.color().view().get(),
                this->m_sampler_man.sampler_tex().get(),
                this->m_desc_layout_man.layout_final(),
                this->m_logi_device.get()
            );

            this->m_cmd_man.record_final(
                this->m_flight_frame_index.get(),
                this->m_fbuf_man.fbuf_final_at(swapchain_index),
                this->m_swapchain.identity_extent(),
                this->m_desc_man.desc_set_final_at(this->m_flight_frame_index.get()),
                this->m_pipelines.final().layout(),
                this->m_pipelines.final().pipeline(),
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

        this->m_flight_frame_index.increase();
    }

    void VulkanState::wait_idle() {
        vkDeviceWaitIdle(this->m_logi_device.get());
    }

    void VulkanState::on_screen_resize(const unsigned width, const unsigned height) {
        this->m_screen_resize_notified = true;
        this->m_new_extent.width = width;
        this->m_new_extent.height = height;

        dalVerbose(fmt::format("Screen resized: {} x {}", this->m_new_extent.width, this->m_new_extent.height).c_str());
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
        auto& a = reinterpret_cast<ActorVK&>(actor);

        a.init(
            this->m_desc_allocator,
            this->m_desc_layout_man.layout_per_actor(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        return true;
    }

    bool VulkanState::init(IActorSkinned& actor) {
        auto& a = reinterpret_cast<ActorSkinnedVK&>(actor);

        a.init(
            this->m_desc_allocator,
            this->m_desc_layout_man.layout_per_actor(),
            this->m_desc_layout_man.layout_animation(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        return true;
    }

    bool VulkanState::prepare(IRenModel& h_model) {
        auto& model = reinterpret_cast<ModelRenderer&>(h_model);

        return model.fetch_one_resource(
            this->m_desc_layout_man.layout_per_material(),
            this->m_sampler_man.sampler_tex().get(),
            this->m_logi_device.get()
        );
    }

    bool VulkanState::prepare(IRenModelSkineed& model) {
        auto& m = reinterpret_cast<ModelSkinnedRenderer&>(model);

        return m.fetch_one_resource(
            this->m_desc_layout_man.layout_per_material(),
            this->m_sampler_man.sampler_tex().get(),
            this->m_logi_device.get()
        );
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

        this->m_attach_man.init(
            ::calc_smaller_extent(this->m_new_extent, 0.9),
            this->m_phys_device.find_depth_format(),
            this->m_phys_device.get(),
            this->m_logi_device.get()
        );

        this->m_renderpasses.init(
            this->m_swapchain.format(),
            this->m_attach_man.color().format(),
            this->m_attach_man.depth().format(),
            this->m_attach_man.albedo().format(),
            this->m_attach_man.materials().format(),
            this->m_attach_man.normal().format(),
            this->m_logi_device.get()
        );

        for (auto& x : this->m_dlight_shadow_maps) {
            x.init(
                2048*2, 2048*2,
                dal::MAX_FRAMES_IN_FLIGHT,
                this->m_logi_device.indices().graphics_family(),
                this->m_renderpasses.rp_shadow(),
                this->m_phys_device.find_depth_format(),
                this->m_phys_device.get(),
                this->m_logi_device.get()
            );
        }

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
            this->m_filesys.asset_mgr(),
            !this->m_swapchain.is_format_srgb(),
            this->m_swapchain.identity_extent(),
            this->m_attach_man.color().extent(),
            this->m_desc_layout_man.layout_final(),
            this->m_desc_layout_man.layout_per_global(),
            this->m_desc_layout_man.layout_per_material(),
            this->m_desc_layout_man.layout_per_actor(),
            this->m_desc_layout_man.layout_animation(),
            this->m_desc_layout_man.layout_composition(),
            this->m_renderpasses.rp_gbuf(),
            this->m_renderpasses.rp_final(),
            this->m_renderpasses.rp_alpha(),
            this->m_renderpasses.rp_shadow(),
            this->m_logi_device.get()
        );

        this->m_cmd_man.init(
            MAX_FRAMES_IN_FLIGHT,
            this->m_logi_device.indices().graphics_family(),
            this->m_logi_device.get()
        );

        this->m_ubuf_man.init(this->m_phys_device.get(), this->m_logi_device.get());

        U_PerFrame_InFinal data;
        data.m_rotation = this->m_swapchain.pre_ratation_mat();
        this->m_ubuf_man.m_ub_final.copy_to_buffer(data, this->m_logi_device.get());

        this->m_desc_man.init(MAX_FRAMES_IN_FLIGHT, this->m_logi_device.get());

        this->m_desc_man.init_desc_sets_per_global(
            this->m_ubuf_man.m_ub_simple,
            this->m_ubuf_man.m_ub_glights,
            MAX_FRAMES_IN_FLIGHT,
            this->m_desc_layout_man.layout_per_global(),
            this->m_logi_device.get()
        );

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            this->m_desc_man.add_desc_set_composition(
                {
                    this->m_attach_man.depth().view().get(),
                    this->m_attach_man.albedo().view().get(),
                    this->m_attach_man.materials().view().get(),
                    this->m_attach_man.normal().view().get(),
                },
                this->m_ubuf_man.m_ub_glights.at(i),
                this->m_ubuf_man.m_ub_per_frame_composition.at(i),
                {
                    this->m_dlight_shadow_maps[0].shadow_map_view(),
                    this->m_dlight_shadow_maps[1].shadow_map_view(),
                },
                this->m_sampler_man.sampler_tex().get(),
                this->m_desc_layout_man.layout_composition(),
                this->m_logi_device.get()
            );
        }

        return true;
    }

}
