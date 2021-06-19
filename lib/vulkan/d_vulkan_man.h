#pragma once

#include <vector>
#include <functional>

#include "d_actor.h"
#include "d_renderer.h"
#include "d_vk_device.h"
#include "d_filesystem.h"
#include "d_task_thread.h"
#include "d_vk_managers.h"


#define DAL_VK_DEBUG


namespace dal {

    class ShadowMapFbuf {

    private:
        CommandPool m_cmd_pool;
        FbufAttachment m_depth_attach;
        Fbuf_Shadow m_fbuf;
        std::vector<VkCommandBuffer> m_cmd_buf;

    public:
        void init(
            const uint32_t width,
            const uint32_t height,
            const uint32_t max_in_flight_count,
            const uint32_t queue_fam_index,
            const RenderPass_ShadowMap& rp_shadow,
            const VkFormat depth_format,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        ) {
            this->m_depth_attach.init(width, height, dal::FbufAttachment::Usage::depth_attachment, depth_format, phys_device, logi_device);
            this->m_fbuf.init(rp_shadow, VkExtent2D{width, height}, this->m_depth_attach.view().get(), logi_device);
            this->m_cmd_pool.init(queue_fam_index, logi_device);
            this->m_cmd_buf = this->m_cmd_pool.allocate(max_in_flight_count, logi_device);
        }

        void destroy(const VkDevice logi_device) {
            this->m_cmd_pool.free(this->m_cmd_buf, logi_device);
            this->m_cmd_pool.destroy(logi_device);
            this->m_fbuf.destroy(logi_device);
            this->m_depth_attach.destroy(logi_device);
        }

    };


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
        ShadowMapFbuf m_shadow_map;

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
