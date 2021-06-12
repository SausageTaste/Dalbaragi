#include "d_vk_managers.h"

#include "d_logger.h"


namespace dal {

    void CmdPoolManager::init(const uint32_t swapchain_count, const uint32_t queue_family_index, const VkDevice logi_device) {
        this->destroy(logi_device);

        this->m_pools.resize(swapchain_count);

        this->m_cmd_simple.resize(swapchain_count);
        this->m_cmd_final.resize(swapchain_count);
        this->m_cmd_alpha.resize(swapchain_count);

        for (size_t i = 0; i < this->m_pools.size(); ++i) {
            this->m_pools[i].init(queue_family_index, logi_device);
            this->m_pools[i].allocate(&this->m_cmd_simple[i], 1, logi_device);
            this->m_pools[i].allocate(&this->m_cmd_final[i], 1, logi_device);
            this->m_pools[i].allocate(&this->m_cmd_alpha[i], 1, logi_device);
        }
    }

    void CmdPoolManager::destroy(const VkDevice logi_device) {
        for (auto& pool : this->m_pools)
            pool.destroy(logi_device);
        this->m_pools.clear();

        this->m_cmd_simple.clear();
        this->m_cmd_final.clear();
        this->m_cmd_alpha.clear();
    }

    void CmdPoolManager::record_simple(
        const size_t flight_frame_index,
        const std::vector<ModelRenderer*>& models,
        const VkDescriptorSet desc_set_per_frame,
        const VkDescriptorSet desc_set_composition,
        const VkExtent2D& swapchain_extent,
        const VkFramebuffer swapchain_fbuf,
        const VkPipeline pipeline_gbuf,
        const VkPipelineLayout pipe_layout_gbuf,
        const VkPipeline pipeline_composition,
        const VkPipelineLayout pipe_layout_composition,
        const RenderPass_Gbuf& render_pass
    ) {
        auto& cmd_buf = this->m_cmd_simple.at(flight_frame_index);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(cmd_buf, &begin_info) != VK_SUCCESS) {
            dalAbort("failed to begin recording command buffer!");
        }

        std::array<VkClearValue, 5> clear_colors{};
        clear_colors[0].color = { 0, 0, 0, 1 };
        clear_colors[1].depthStencil = { 1, 0 };
        clear_colors[2].color = { 0, 0, 0, 1 };
        clear_colors[3].color = { 0, 0, 0, 1 };
        clear_colors[4].color = { 0, 0, 0, 1 };

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass.get();
        render_pass_info.framebuffer = swapchain_fbuf;
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swapchain_extent;
        render_pass_info.clearValueCount = clear_colors.size();
        render_pass_info.pClearValues = clear_colors.data();

        vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_gbuf);

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& model : models) {
                if (!model->is_ready())
                    continue;

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipe_layout_gbuf,
                    2,
                    1, &model->desc_set_per_actor().get(),
                    0, nullptr
                );

                for (auto& unit : model->render_units()) {
                    if (unit.m_alpha_blend)
                        continue;

                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipe_layout_gbuf,
                        0,
                        1, &desc_set_per_frame,
                        0, nullptr
                    );

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipe_layout_gbuf,
                        1,
                        1, &unit.m_desc_set.get(),
                        0, nullptr
                    );

                    vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                }
            }
        }
        {
            vkCmdNextSubpass(cmd_buf, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_composition);

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipe_layout_composition,
                0,
                1, &desc_set_composition,
                0, nullptr
            );

            vkCmdDraw(cmd_buf, 6, 1, 0, 0);
        }
        vkCmdEndRenderPass(cmd_buf);

        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
            dalAbort("failed to record command buffer!");
        }

    }

    void CmdPoolManager::record_final(
        const size_t index,
        const dal::Fbuf_Final& fbuf,
        const VkExtent2D& extent,
        const VkDescriptorSet desc_set_final,
        const VkPipelineLayout pipe_layout_final,
        const VkPipeline pipeline_final,
        const RenderPass_Final& renderpass
    ) {
        auto& cmd_buf = this->m_cmd_final.at(index);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(cmd_buf, &begin_info) != VK_SUCCESS) {
            dalAbort("failed to begin recording command buffer!");
        }

        std::array<VkClearValue, 1> clear_colors{};
        clear_colors[0].color = { 1, 0, 0, 1 };

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = renderpass.get();
        render_pass_info.framebuffer = fbuf.get();
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = extent;
        render_pass_info.clearValueCount = clear_colors.size();
        render_pass_info.pClearValues = clear_colors.data();

        vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_final);

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipe_layout_final,
                0,
                1, &desc_set_final,
                0, nullptr
            );

            vkCmdDraw(cmd_buf, 6, 1, 0, 0);
        }
        vkCmdEndRenderPass(cmd_buf);

        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
            dalAbort("failed to record command buffer!");
        }

    }

    void CmdPoolManager::record_alpha(
        const size_t flight_frame_index,
        const std::vector<ModelRenderer*>& models,
        const VkDescriptorSet desc_set_per_frame,
        const VkDescriptorSet desc_set_composition,
        const VkExtent2D& swapchain_extent,
        const VkFramebuffer swapchain_fbuf,
        const VkPipeline pipeline_alpha,
        const VkPipelineLayout pipe_layout_alpha,
        const RenderPass_Alpha& render_pass
    ) {
        auto& cmd_buf = this->m_cmd_alpha.at(flight_frame_index);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(cmd_buf, &begin_info) != VK_SUCCESS) {
            dalAbort("failed to begin recording command buffer!");
        }

        std::array<VkClearValue, 5> clear_colors{};
        clear_colors[0].color = { 0, 0, 0, 1 };
        clear_colors[1].depthStencil = { 1, 0 };
        clear_colors[2].color = { 0, 0, 0, 1 };
        clear_colors[3].color = { 0, 0, 0, 1 };
        clear_colors[4].color = { 0, 0, 0, 1 };

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass.get();
        render_pass_info.framebuffer = swapchain_fbuf;
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swapchain_extent;
        render_pass_info.clearValueCount = clear_colors.size();
        render_pass_info.pClearValues = clear_colors.data();

        vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_alpha);

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& model : models) {
                if (!model->is_ready())
                    continue;

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipe_layout_alpha,
                    2,
                    1, &model->desc_set_per_actor().get(),
                    0, nullptr
                );

                for (auto& unit : model->render_units()) {
                    if (!unit.m_alpha_blend)
                        continue;

                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipe_layout_alpha,
                        0,
                        1, &desc_set_per_frame,
                        0, nullptr
                    );

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipe_layout_alpha,
                        1,
                        1, &unit.m_desc_set.get(),
                        0, nullptr
                    );

                    vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                }
            }
        }
        vkCmdEndRenderPass(cmd_buf);

        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
            dalAbort("failed to record command buffer!");
        }
    }

}
