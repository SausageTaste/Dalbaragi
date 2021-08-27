#pragma once

#include <vector>
#include <functional>

#include "d_actor.h"
#include "d_renderer.h"
#include "d_filesystem.h"
#include "d_task_thread.h"
#include "d_vk_managers.h"


#define DAL_VK_DEBUG


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
        DescAllocator m_desc_allocator;
        ShadowMapManager m_shadow_maps;

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
        uint64_t m_frame_count = 0;

    public:
        VulkanState(
            const char* const window_title,
            const unsigned init_width,
            const unsigned init_height,
            dal::Filesystem& filesys,
            dal::TaskManager& task_man,
            dal::ITextureManager& texture_man,
            const std::vector<const char*>& extensions,
            surface_create_func_t surface_create_func
        );

        ~VulkanState();

        void update(const ICamera& camera, RenderList& render_list) override;

        const FrameInFlightIndex& in_flight_index() const override {
            return this->m_flight_frame_index;
        }

        void wait_idle() override;

        void on_screen_resize(const unsigned width, const unsigned height) override;

        HTexture create_texture() override;

        HRenModel create_model() override;

        HRenModelSkinned create_model_skinned() override;

        HActor create_actor() override;

        HActorSkinned create_actor_skinned() override;

        bool init(ITexture& tex, const ImageData& img_data) override;

        bool init(IRenModel& model, const dal::ModelStatic& model_data, const char* const fallback_namespace) override;

        bool init(IRenModelSkineed& model, const dal::ModelSkinned& model_data, const char* const fallback_namespace) override;

        bool init(IActor& actor) override;

        bool init(IActorSkinned& actor) override;

        bool prepare(IRenModel& model) override;

        bool prepare(IRenModelSkineed& model) override;

    private:
        // Returns true if recreation is still needed.
        bool on_recreate_swapchain();

        [[nodiscard]]
        bool init_swapchain_and_dependers();

    };

}
