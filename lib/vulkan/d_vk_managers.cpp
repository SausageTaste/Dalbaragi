#include "d_vk_managers.h"

#include "d_logger.h"


// CmdPoolManager
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

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipe_layout_gbuf,
                0,
                1, &desc_set_per_frame,
                0, nullptr
            );

            for (auto& render_pair : render_list) {
                if (!render_pair.m_model->is_ready())
                    continue;

                auto& model = *reinterpret_cast<ModelRenderer*>(render_pair.m_model.get());

                for (auto& unit : model.render_units()) {
                    dalAssert(!unit.m_material.m_alpha_blend);

                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipe_layout_gbuf,
                        1,
                        1, &unit.m_material.m_descset.get(),
                        0, nullptr
                    );

                    for (auto& h_actor : render_pair.m_actors) {
                        auto& actor = *reinterpret_cast<ActorVK*>(h_actor.get());
                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipe_layout_gbuf,
                            2,
                            1, &actor.desc_set_raw(),
                            0, nullptr
                        );

                        vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                    }
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
        const RenderList& render_list,
        const VkDescriptorSet desc_set_per_frame,
        const VkDescriptorSet desc_set_per_world,
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
                3,
                1, &desc_set_per_world,
                0, nullptr
            );

            for (auto& render_pair : render_list) {
                if (!render_pair.m_model->is_ready())
                    continue;

                auto& model = *reinterpret_cast<ModelRenderer*>(render_pair.m_model.get());
                for (auto& unit : model.render_units_alpha()) {
                    dalAssert(unit.m_material.m_alpha_blend);

                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipe_layout_alpha,
                        1,
                        1, &unit.m_material.m_descset.get(),
                        0, nullptr
                    );

                    for (auto& h_actor : render_pair.m_actors) {
                        auto actor = reinterpret_cast<ActorVK*>(h_actor.get());
                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipe_layout_alpha,
                            2,
                            1, &actor->desc_set_raw(),
                            0, nullptr
                        );

                        vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                    }


                }
            }
        }
        vkCmdEndRenderPass(cmd_buf);

        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
            dalAbort("failed to record command buffer!");
        }
    }

}


//
namespace dal {

    void FbufManager::init(
        const std::vector<dal::ImageView>& swapchain_views,
        const dal::AttachmentManager& attach_man,
        const VkExtent2D& swapchain_extent,
        const VkExtent2D& gbuf_extent,
        const dal::RenderPass_Gbuf& rp_gbuf,
        const dal::RenderPass_Final& rp_final,
        const dal::RenderPass_Alpha& rp_alpha,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        for (uint32_t i = 0; i < swapchain_views.size(); ++i) {
            this->m_fbuf_simple.emplace_back().init(
                rp_gbuf,
                gbuf_extent,
                attach_man.color().view().get(),
                attach_man.depth().view().get(),
                attach_man.albedo().view().get(),
                attach_man.materials().view().get(),
                attach_man.normal().view().get(),
                logi_device
            );

            this->m_fbuf_final.emplace_back().init(
                rp_final,
                swapchain_extent,
                swapchain_views.at(i).get(),
                logi_device
            );

            this->m_fbuf_alpha.emplace_back().init(
                rp_alpha,
                gbuf_extent,
                attach_man.color().view().get(),
                attach_man.depth().view().get(),
                logi_device
            );
        }
    }

    void FbufManager::destroy(const VkDevice logi_device) {
        for (auto& fbuf : this->m_fbuf_simple)
            fbuf.destroy(logi_device);
        this->m_fbuf_simple.clear();

        for (auto& fbuf : this->m_fbuf_final)
            fbuf.destroy(logi_device);
        this->m_fbuf_final.clear();

        for (auto& fbuf : this->m_fbuf_alpha)
            fbuf.destroy(logi_device);
        this->m_fbuf_alpha.clear();
    }

    std::vector<VkFramebuffer> FbufManager::swapchain_fbuf() const {
        std::vector<VkFramebuffer> output;

        for (auto& x : this->m_fbuf_simple) {
            output.push_back(x.get());
        }

        return output;
    }

}


// UbufManager
namespace dal {

    void UbufManager::init(
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_ub_simple.init(
            MAX_FRAMES_IN_FLIGHT,
            phys_device,
            logi_device
        );

        this->m_ub_glights.init(
            MAX_FRAMES_IN_FLIGHT,
            phys_device,
            logi_device
        );

        this->m_ub_per_frame_composition.init(
            MAX_FRAMES_IN_FLIGHT,
            phys_device,
            logi_device
        );

        this->m_ub_per_frame_alpha.init(
            MAX_FRAMES_IN_FLIGHT,
            phys_device,
            logi_device
        );

        this->m_ub_final.init(phys_device, logi_device);
    }

    void UbufManager::destroy(const VkDevice logi_device) {
        this->m_ub_simple.destroy(logi_device);
        this->m_ub_glights.destroy(logi_device);
        this->m_ub_per_frame_composition.destroy(logi_device);
        this->m_ub_per_frame_alpha.destroy(logi_device);
        this->m_ub_final.destroy(logi_device);
    }

}
