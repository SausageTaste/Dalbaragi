#pragma once

#include <vector>
#include <functional>

#include "d_actor.h"
#include "d_shader.h"
#include "d_renderer.h"
#include "d_vk_device.h"
#include "d_filesystem.h"
#include "d_task_thread.h"
#include "d_vk_managers.h"


#ifndef DAL_OS_ANDROID
    #define DAL_VK_DEBUG
#endif


namespace dal {

    class VulkanState : public IRenderer {

    private:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        PhysicalDevice m_phys_device;
        PhysDeviceInfo m_phys_info;
        LogicalDevice m_logi_device;

        SwapchainManager m_swapchain;
        PipelineManager m_pipelines;
        AttachmentManager m_attach_man;
        RenderPassManager m_renderpasses;
        FbufManager m_fbuf_man;
        CmdPoolManager m_cmd_man;
        DescSetLayoutManager m_desc_layout_man;
        UbufManager m_ubuf_man;
        DescriptorManager m_desc_man;

        SamplerManager m_sampler_man;
        DescPool m_desc_pool_actor;

    private:
        // Non-vulkan members
        dal::Filesystem& m_filesys;
        ITextureManager& m_texture_man;

#ifdef DAL_VK_DEBUG
        VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
#endif

        FrameInFlightIndex m_flight_frame_index;
        bool m_screen_resize_notified = false;
        VkExtent2D m_new_extent;

    public:
        VulkanState(
            const char* const window_title,
            const unsigned init_width,
            const unsigned init_height,
            dal::Filesystem& filesys,
            dal::TaskManager& task_man,
            dal::ITextureManager& texture_man,
            const std::vector<const char*>& extensions,
            std::function<void*(void*)> surface_create_func
        );

        ~VulkanState();

        void update(const ICamera& camera, const RenderList& render_list) override;

        void wait_idle() override;

        void on_screen_resize(const unsigned width, const unsigned height) override;

        HTexture create_texture() override;

        HRenModel create_model();

        HActor create_actor() override;

        bool init_texture(ITexture& tex, const ImageData& img_data) override;

        bool init_model(IRenModel& model, const dal::ModelStatic& model_data, const char* const fallback_namespace) override;

        bool prepare(IRenModel& model) override;

    private:
        // Returns true if recreation is still needed.
        bool on_recreate_swapchain();

        [[nodiscard]]
        bool init_swapchain_and_dependers();

    };

}
