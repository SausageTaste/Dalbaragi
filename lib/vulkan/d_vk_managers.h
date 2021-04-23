#pragma once

#include <vector>

#include "d_command.h"
#include "d_model_renderer.h"
#include "d_render_pass.h"
#include "d_framebuffer.h"


namespace dal {

    class CmdPoolManager {

    private:
        std::vector<CommandPool> m_pools;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_simple;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_final;  // Per swapchain
        CommandPool m_pool_for_single_time;

    public:
        void init(const uint32_t swapchain_count, const uint32_t queue_family_index, const VkDevice logi_device);

        void destroy(const VkDevice logi_device);

        void record_simple(
            const size_t flight_frame_index,
            const std::vector<ModelRenderer*>& models,
            const std::vector<VkDescriptorSet>& desc_sets_simple,
            const VkExtent2D& swapchain_extent,
            const VkFramebuffer swapchain_fbuf,
            const VkPipelineLayout pipe_layout_simple,
            const VkPipeline graphics_pipeline,
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

        auto& cmd_simple_at(const size_t index) const {
            return this->m_cmd_simple.at(index);
        }

        auto& cmd_final_at(const size_t index) const {
            return this->m_cmd_final.at(index);
        }

        auto& pool_single_time() {
            return this->m_pool_for_single_time;
        }

    };

}
