#pragma once

#include <vector>

#include "d_konsts.h"
#include "d_command.h"
#include "d_swapchain.h"
#include "d_render_pass.h"
#include "d_framebuffer.h"
#include "d_model_renderer.h"


namespace dal {

    class CmdPoolManager {

    private:
        std::vector<CommandPool> m_pools;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_simple;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_final;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_alpha;  // Per swapchain

    public:
        void init(const uint32_t swapchain_count, const uint32_t queue_family_index, const VkDevice logi_device);

        void destroy(const VkDevice logi_device);

        void record_simple(
            const size_t flight_frame_index,
            const RenderList& render_list,
            const VkDescriptorSet desc_set_per_frame,
            const VkDescriptorSet desc_set_composition,
            const VkExtent2D& swapchain_extent,
            const VkFramebuffer swapchain_fbuf,
            const VkPipeline pipeline_gbuf,
            const VkPipelineLayout pipe_layout_gbuf,
            const VkPipeline pipeline_composition,
            const VkPipelineLayout pipe_layout_composition,
            const RenderPass_Gbuf& render_pass
        );

        void record_final(
            const size_t index,
            const dal::Fbuf_Final& fbuf,
            const VkExtent2D& extent,
            const VkDescriptorSet desc_set_final,
            const VkPipelineLayout pipe_layout_final,
            const VkPipeline pipeline_final,
            const RenderPass_Final& renderpass
        );

        void record_alpha(
            const size_t flight_frame_index,
            const RenderList& render_list,
            const VkDescriptorSet desc_set_per_frame,
            const VkDescriptorSet desc_set_per_world,
            const VkDescriptorSet desc_set_composition,
            const VkExtent2D& swapchain_extent,
            const VkFramebuffer swapchain_fbuf,
            const VkPipeline pipeline_alpha,
            const VkPipelineLayout pipe_layout_alpha,
            const RenderPass_Alpha& render_pass
        );

        auto& cmd_simple_at(const size_t index) const {
            return this->m_cmd_simple.at(index);
        }

        auto& cmd_final_at(const size_t index) const {
            return this->m_cmd_final.at(index);
        }

        auto& cmd_alpha_at(const size_t index) const {
            return this->m_cmd_alpha.at(index);
        }

    };


    class FbufManager {

    private:
        std::vector<dal::Fbuf_Simple> m_fbuf_simple;
        std::vector<dal::Fbuf_Final> m_fbuf_final;
        std::vector<dal::Fbuf_Alpha> m_fbuf_alpha;

    public:
        void init(
            const std::vector<dal::ImageView>& swapchain_views,
            const dal::AttachmentManager& attach_man,
            const VkExtent2D& swapchain_extent,
            const VkExtent2D& gbuf_extent,
            const dal::RenderPass_Gbuf& rp_gbuf,
            const dal::RenderPass_Final& rp_final,
            const dal::RenderPass_Alpha& rp_alpha,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        std::vector<VkFramebuffer> swapchain_fbuf() const;

        auto& fbuf_final_at(const dal::SwapchainIndex index) const {
            return this->m_fbuf_final.at(*index);
        }

        auto& fbuf_alpha_at(const dal::SwapchainIndex index) const {
            return this->m_fbuf_alpha.at(*index);
        }

    };


    class UbufManager {

    public:
        UniformBufferArray<U_PerFrame> m_ub_simple;
        UniformBufferArray<U_GlobalLight> m_ub_glights;
        UniformBufferArray<U_PerFrame_Composition> m_ub_per_frame_composition;
        UniformBufferArray<U_PerFrame_Alpha> m_ub_per_frame_alpha;
        UniformBuffer<U_PerFrame_InFinal> m_ub_final;

    public:
        void init(
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

    };

}
