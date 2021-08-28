#include "d_vk_managers.h"

#include "d_logger.h"


namespace {

    struct AlphaMeshTuple {
        const dal::RenderUnit* m_unit;
        const dal::ActorVK* m_actor;
        float m_dist_spr;

        bool operator<(const AlphaMeshTuple& other) const {
            return this->m_dist_spr > other.m_dist_spr;
        }
    };

    struct AnimatedAlphaMeshTuple {
        const dal::RenderUnit* m_unit;
        const dal::ActorSkinnedVK* m_actor;
        float m_dist_spr;

        bool operator<(const AnimatedAlphaMeshTuple& other) const {
            return this->m_dist_spr > other.m_dist_spr;
        }
    };

    auto sort_transparent_meshes(const glm::vec3& view_pos, const dal::RenderList& render_list) {
        std::vector<AlphaMeshTuple> sort_vec;
        std::vector<AnimatedAlphaMeshTuple> sort_vec_animated;

        for (auto& pair : render_list.m_static_models) {
            auto& model = *reinterpret_cast<dal::ModelRenderer*>(pair.m_model.get());

            for (auto& h_actor : pair.m_actors) {
                auto& actor = *reinterpret_cast<dal::ActorVK*>(h_actor.get());
                const auto actor_transform = actor.m_transform.make_mat4();

                for (auto& unit : model.render_units_alpha()) {
                    auto& sort_element = sort_vec.emplace_back();
                    const auto unit_world_pos = actor_transform * glm::vec4(unit.m_weight_center, 1);
                    const auto to_view = view_pos - glm::vec3(unit_world_pos);

                    sort_element.m_unit = &unit;
                    sort_element.m_actor = &actor;
                    sort_element.m_dist_spr = glm::dot(to_view, to_view);
                }
            }
        }

        for (auto& pair : render_list.m_skinned_models) {
            auto& model = *reinterpret_cast<dal::ModelSkinnedRenderer*>(pair.m_model.get());

            for (auto& h_actor : pair.m_actors) {
                auto& actor = *reinterpret_cast<dal::ActorSkinnedVK*>(h_actor.get());
                const auto actor_transform = actor.m_transform.make_mat4();

                for (auto& unit : model.render_units_alpha()) {
                    auto& sort_element = sort_vec_animated.emplace_back();
                    const auto unit_world_pos = actor_transform * glm::vec4(unit.m_weight_center, 1);
                    const auto to_view = view_pos - glm::vec3(unit_world_pos);

                    sort_element.m_unit = &unit;
                    sort_element.m_actor = &actor;
                    sort_element.m_dist_spr = glm::dot(to_view, to_view);
                }
            }
        }

        std::sort(sort_vec.begin(), sort_vec.end());
        std::sort(sort_vec_animated.begin(), sort_vec_animated.end());
        return std::make_pair(sort_vec, sort_vec_animated);
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


// CmdPoolManager
namespace dal {

    void CmdPoolManager::init(const uint32_t swapchain_count, const uint32_t queue_family_index, const VkDevice logi_device) {
        this->destroy(logi_device);

        this->m_general_pool.init(queue_family_index, logi_device);

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

        this->m_general_pool.destroy(logi_device);

        this->m_cmd_simple.clear();
        this->m_cmd_final.clear();
        this->m_cmd_alpha.clear();
    }

    void CmdPoolManager::record_simple(
        const FrameInFlightIndex& flight_frame_index,
        const RenderList& render_list,
        const VkDescriptorSet desc_set_per_frame,
        const VkDescriptorSet desc_set_composition,
        const VkExtent2D& swapchain_extent,
        const VkFramebuffer swapchain_fbuf,
        const ShaderPipeline& pipeline_gbuf,
        const ShaderPipeline& pipeline_gbuf_animated,
        const ShaderPipeline& pipeline_composition,
        const RenderPass_Gbuf& render_pass
    ) {
        auto& cmd_buf = this->m_cmd_simple.at(flight_frame_index.get());

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(cmd_buf, &begin_info) != VK_SUCCESS)
            dalAbort("failed to begin recording command buffer!");

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

        // Gbuf of static models
        {
            auto& pipeline = pipeline_gbuf;

            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout(),
                0,
                1, &desc_set_per_frame,
                0, nullptr
            );

            for (auto& render_pair : render_list.m_static_models) {
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
                        pipeline.layout(),
                        1,
                        1, &unit.m_material.m_descset.get(),
                        0, nullptr
                    );

                    for (auto& h_actor : render_pair.m_actors) {
                        auto& actor = *reinterpret_cast<ActorVK*>(h_actor.get());
                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            2,
                            1, &actor.desc_set_raw(),
                            0, nullptr
                        );

                        vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                    }
                }
            }
        }

        // Gbuf of animated models
        {
            auto& pipeline = pipeline_gbuf_animated;

            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout(),
                0,
                1, &desc_set_per_frame,
                0, nullptr
            );

            for (auto& render_pair : render_list.m_skinned_models) {
                if (!render_pair.m_model->is_ready())
                    continue;

                auto& model = *reinterpret_cast<ModelSkinnedRenderer*>(render_pair.m_model.get());

                for (auto& unit : model.render_units()) {
                    dalAssert(!unit.m_material.m_alpha_blend);

                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline.layout(),
                        1,
                        1, &unit.m_material.m_descset.get(),
                        0, nullptr
                    );

                    for (auto& h_actor : render_pair.m_actors) {
                        auto& actor = *reinterpret_cast<ActorSkinnedVK*>(h_actor.get());

                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            2,
                            1, &actor.desc_per_actor(flight_frame_index),
                            0, nullptr
                        );

                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            3,
                            1, &actor.desc_animation(flight_frame_index),
                            0, nullptr
                        );

                        vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                    }
                }
            }
        }

        // Composition
        {
            auto& pipeline = pipeline_composition;

            vkCmdNextSubpass(cmd_buf, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout(),
                0,
                1, &desc_set_composition,
                0, nullptr
            );

            vkCmdDraw(cmd_buf, 6, 1, 0, 0);
        }

        vkCmdEndRenderPass(cmd_buf);
        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS)
            dalAbort("failed to record command buffer!");
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

            vkCmdDraw(cmd_buf, 3, 1, 0, 0);
        }
        vkCmdEndRenderPass(cmd_buf);

        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
            dalAbort("failed to record command buffer!");
        }

    }

    void CmdPoolManager::record_alpha(
        const FrameInFlightIndex& flight_frame_index,
        const glm::vec3& view_pos,
        const RenderList& render_list,
        const VkDescriptorSet desc_set_per_global,
        const VkDescriptorSet desc_set_composition,
        const VkExtent2D& swapchain_extent,
        const VkFramebuffer swapchain_fbuf,
        const ShaderPipeline& pipeline_alpha,
        const ShaderPipeline& pipeline_alpha_animated,
        const RenderPass_Alpha& render_pass
    ) {
        auto& cmd_buf = this->m_cmd_alpha.at(flight_frame_index.get());

        const auto [sorted, sorted_animated] = ::sort_transparent_meshes(view_pos, render_list);

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
            auto& pipeline = pipeline_alpha;

            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout(),
                0,
                1, &desc_set_per_global,
                0, nullptr
            );

            for (auto& render_tuple : sorted) {
                if (!render_tuple.m_unit->is_ready())
                    continue;

                std::array<VkBuffer, 1> vert_bufs{ render_tuple.m_unit->m_vert_buffer.vertex_buffer() };
                vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                vkCmdBindIndexBuffer(cmd_buf, render_tuple.m_unit->m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.layout(),
                    1,
                    1, &render_tuple.m_unit->m_material.m_descset.get(),
                    0, nullptr
                );

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.layout(),
                    2,
                    1, &render_tuple.m_actor->desc_set_raw(),
                    0, nullptr
                );

                vkCmdDrawIndexed(cmd_buf, render_tuple.m_unit->m_vert_buffer.index_size(), 1, 0, 0, 0);
            }
        }

        {
            auto& pipeline = pipeline_alpha_animated;

            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout(),
                0,
                1, &desc_set_per_global,
                0, nullptr
            );

            for (auto& render_tuple : sorted_animated) {
                if (!render_tuple.m_unit->is_ready())
                    continue;

                std::array<VkBuffer, 1> vert_bufs{ render_tuple.m_unit->m_vert_buffer.vertex_buffer() };
                vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                vkCmdBindIndexBuffer(cmd_buf, render_tuple.m_unit->m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.layout(),
                    1,
                    1, &render_tuple.m_unit->m_material.m_descset.get(),
                    0, nullptr
                );

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.layout(),
                    2,
                    1, &render_tuple.m_actor->desc_per_actor(flight_frame_index),
                    0, nullptr
                );

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.layout(),
                    3,
                    1, &render_tuple.m_actor->desc_animation(flight_frame_index),
                    0, nullptr
                );

                vkCmdDrawIndexed(cmd_buf, render_tuple.m_unit->m_vert_buffer.index_size(), 1, 0, 0, 0);
            }
        }

        vkCmdEndRenderPass(cmd_buf);

        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
            dalAbort("failed to record command buffer!");
        }
    }

}


// FbufManager
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

        this->m_ub_final.init(phys_device, logi_device);
    }

    void UbufManager::destroy(const VkDevice logi_device) {
        this->m_ub_simple.destroy(logi_device);
        this->m_ub_glights.destroy(logi_device);
        this->m_ub_per_frame_composition.destroy(logi_device);
        this->m_ub_final.destroy(logi_device);
    }

}


// ShadowMap
namespace dal {

    void ShadowMap::init(
        const uint32_t width,
        const uint32_t height,
        const uint32_t max_in_flight_count,
        CommandPool& cmd_pool,
        const RenderPass_ShadowMap& rp_shadow,
        const VkFormat depth_format,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(cmd_pool, logi_device);

        this->m_depth_attach.init(width, height, dal::FbufAttachment::Usage::depth_map, depth_format, phys_device, logi_device);
        this->m_fbuf.init(rp_shadow, VkExtent2D{width, height}, this->m_depth_attach.view().get(), logi_device);
        this->m_cmd_buf = cmd_pool.allocate(max_in_flight_count, logi_device);
    }

    void ShadowMap::destroy(CommandPool& cmd_pool, const VkDevice logi_device) {
        if (!this->m_cmd_buf.empty()) {
            cmd_pool.free(this->m_cmd_buf, logi_device);
            this->m_cmd_buf.clear();
        }

        this->m_fbuf.destroy(logi_device);
        this->m_depth_attach.destroy(logi_device);
    }

    void ShadowMap::record_cmd_buf(
        const FrameInFlightIndex& flight_frame_index,
        const RenderList& render_list,
        const glm::mat4& light_mat,
        const ShaderPipeline& pipeline_shadow,
        const ShaderPipeline& pipeline_shadow_animated,
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

        {
            auto& pipeline = pipeline_shadow_animated;
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            const auto [viewport, scissor] = ::create_info_viewport_scissor(this->m_depth_attach.extent());
            vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
            vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& render_tuple : render_list.m_skinned_models) {
                auto& model = *reinterpret_cast<ModelSkinnedRenderer*>(render_tuple.m_model.get());

                for (auto& unit : model.render_units()) {
                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    for (auto& h_actor : render_tuple.m_actors) {
                        auto& actor = *reinterpret_cast<ActorSkinnedVK*>(h_actor.get());

                        U_PC_Shadow pc_data;
                        pc_data.m_model_mat = actor.m_transform.make_mat4();
                        pc_data.m_light_mat = light_mat;
                        vkCmdPushConstants(cmd_buf, pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(U_PC_Shadow), &pc_data);

                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            0,
                            1, &actor.desc_animation(flight_frame_index),
                            0, nullptr
                        );

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


// ShadowMapManager
namespace dal {

    void ShadowMapManager::init(
        const RenderPass_ShadowMap& renderpass,
        const PhysicalDevice& phys_device,
        const LogicalDevice& logi_device
    ) {
        this->destroy(logi_device.get());

        const auto depth_format = phys_device.find_depth_format();
        this->m_cmd_pool.init(logi_device.indices().graphics_family(), logi_device.get());

        constexpr uint32_t DLIGHT_RES = 1024;
        constexpr uint32_t SLIGHT_RES = 512;

        for (auto& x : this->m_dlights) {
            x.init(
                DLIGHT_RES, DLIGHT_RES,
                dal::MAX_FRAMES_IN_FLIGHT,
                this->m_cmd_pool,
                renderpass,
                depth_format,
                phys_device.get(),
                logi_device.get()
            );
        }

        for (auto& x : this->m_slights) {
            x.init(
                SLIGHT_RES, SLIGHT_RES,
                dal::MAX_FRAMES_IN_FLIGHT,
                this->m_cmd_pool,
                renderpass,
                depth_format,
                phys_device.get(),
                logi_device.get()
            );
        }

        for (size_t i = 0; i < dal::MAX_DLIGHT_COUNT; ++i)
            this->m_dlight_views[i] = this->m_dlights[i].shadow_map_view();

        for (size_t i = 0; i < dal::MAX_SLIGHT_COUNT; ++i)
            this->m_slight_views[i] = this->m_slights[i].shadow_map_view();

    }

    void ShadowMapManager::render_empty_for_all(
        const PipelineManager& pipelines,
        const RenderPass_ShadowMap& render_pass,
        const LogicalDevice& logi_device
    ) {

        RenderList render_list;
        FrameInFlightIndex index0{0};
        std::array<VkPipelineStageFlags, 0> wait_stages{};
        std::array<VkSemaphore, 0> wait_semaphores{};
        std::array<VkSemaphore, 0> signal_semaphores{};

        for (size_t i = 0; i < this->m_dlights.size(); ++i) {
            this->m_dlights[i].record_cmd_buf(
                index0,
                render_list,
                glm::mat4{1},
                pipelines.shadow(),
                pipelines.shadow_animated(),
                render_pass
            );

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pCommandBuffers = &this->m_dlights[i].cmd_buf_at(index0.get());
            submit_info.commandBufferCount = 1;
            submit_info.waitSemaphoreCount = wait_semaphores.size();
            submit_info.pWaitSemaphores = wait_semaphores.data();
            submit_info.pWaitDstStageMask = wait_stages.data();
            submit_info.signalSemaphoreCount = signal_semaphores.size();
            submit_info.pSignalSemaphores = signal_semaphores.data();

            const auto submit_result = vkQueueSubmit(
                logi_device.queue_graphics(),
                1,
                &submit_info,
                VK_NULL_HANDLE
            );

            dalAssert(VK_SUCCESS == submit_result);
        }

        for (size_t i = 0; i < this->m_slights.size(); ++i) {
            auto& shadow_map = this->m_slights[i];

            shadow_map.record_cmd_buf(
                index0,
                render_list,
                glm::mat4{1},
                pipelines.shadow(),
                pipelines.shadow_animated(),
                render_pass
            );

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pCommandBuffers = &shadow_map.cmd_buf_at(index0.get());
            submit_info.commandBufferCount = 1;
            submit_info.waitSemaphoreCount = wait_semaphores.size();
            submit_info.pWaitSemaphores = wait_semaphores.data();
            submit_info.pWaitDstStageMask = wait_stages.data();
            submit_info.signalSemaphoreCount = signal_semaphores.size();
            submit_info.pSignalSemaphores = signal_semaphores.data();

            const auto submit_result = vkQueueSubmit(
                logi_device.queue_graphics(),
                1,
                &submit_info,
                VK_NULL_HANDLE
            );

            dalAssert(VK_SUCCESS == submit_result);
        }

        logi_device.wait_idle();
    }

    void ShadowMapManager::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_dlights)
            x.destroy(this->m_cmd_pool, logi_device);

        for (auto& x : this->m_slights)
            x.destroy(this->m_cmd_pool, logi_device);

        for (size_t i = 1; i < dal::MAX_DLIGHT_COUNT; ++i)
            this->m_dlight_views[i] = VK_NULL_HANDLE;

        for (size_t i = 1; i < dal::MAX_SLIGHT_COUNT; ++i)
            this->m_slight_views[i] = VK_NULL_HANDLE;

        this->m_cmd_pool.destroy(logi_device);
    }

    std::array<bool, dal::MAX_DLIGHT_COUNT> ShadowMapManager::create_dlight_update_flags() {
        static_assert(3 == dal::MAX_DLIGHT_COUNT);

        std::array<bool, dal::MAX_DLIGHT_COUNT> output{ false };
        const std::array<float, dal::MAX_DLIGHT_COUNT> intervals{ 0, 0.033, 0.17 };

        for (size_t i = 0; i < dal::MAX_DLIGHT_COUNT; ++i) {
            if (this->m_dlight_timers[i].get_elapsed() > intervals[i]) {
                output[i] = true;
                this->m_dlight_timers[i].check();
            }
        }

        return output;
    }

}
