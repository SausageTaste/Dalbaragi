#include "d_vk_managers.h"

#include <fmt/format.h>

#include "d_logger.h"


namespace {

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


// VulkanResourceManager
namespace dal {

    VulkanResourceManager::~VulkanResourceManager() {
        dalAssert(this->m_textures.empty());
        dalAssert(this->m_models.empty());
        dalAssert(this->m_skinned_models.empty());
        dalAssert(this->m_actors.empty());
        dalAssert(this->m_skinned_actors.empty());
    }

    void VulkanResourceManager::destroy() {
        for (auto& x : this->m_textures) {
            x->destroy();
            x->clear_dependencies();
        }
        this->m_textures.clear();

        for (auto& x : this->m_models) {
            x->destroy();
            x->clear_dependencies();
        }
        this->m_models.clear();

        for (auto& x : this->m_skinned_models) {
            x->destroy();
        }
        this->m_skinned_models.clear();

        for (auto& x : this->m_actors) {
            x->destroy();
            x->clear_dependencies();
        }
        this->m_actors.clear();

        for (auto& x : this->m_skinned_actors) {
            x->destroy();
            x->clear_dependencies();
        }
        this->m_skinned_actors.clear();
    }

    HTexture VulkanResourceManager::create_texture(
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_textures.push_back(std::make_shared<TextureProxy>());
        auto& tex = this->m_textures.back();
        tex->give_dependencies(cmd_pool, graphics_queue, phys_device, logi_device);
        return tex;
    }

    HMesh VulkanResourceManager::create_mesh(
        dal::CommandPool& cmd_pool,
        const VkPhysicalDevice phys_device,
        const dal::LogicalDevice& logi_device
    ) {
        this->m_meshes.push_back(std::make_shared<MeshProxy>());
        auto& mesh = this->m_meshes.back();
        mesh->give_dependencies(cmd_pool, phys_device, logi_device);
        return mesh;
    }

    HRenModel VulkanResourceManager::create_model(
        CommandPool&                  cmd_pool,
        ITextureManager&              tex_man,
        DescLayout_PerActor const&    layout_per_actor,
        DescLayout_PerMaterial const& layout_per_material,
        SamplerTexture const&         sampler,
        VkQueue                       graphics_queue,
        VkPhysicalDevice              phys_device,
        VkDevice                      logi_device
    ) {
        this->m_models.push_back(std::make_shared<ModelProxy>());
        auto& model = this->m_models.back();

        model->give_dependencies(
            cmd_pool,
            tex_man,
            layout_per_actor,
            layout_per_material,
            sampler,
            graphics_queue,
            phys_device,
            logi_device
        );

        return model;
    }

    HRenModelSkinned VulkanResourceManager::create_model_skinned(
        CommandPool&                  cmd_pool,
        ITextureManager&              tex_man,
        DescLayout_PerActor const&    layout_per_actor,
        DescLayout_PerMaterial const& layout_per_material,
        SamplerTexture const&         sampler,
        VkQueue                       graphics_queue,
        VkPhysicalDevice              phys_device,
        VkDevice                      logi_device
    ) {
        this->m_skinned_models.push_back(std::make_shared<ModelSkinnedProxy>());
        auto& model = this->m_skinned_models.back();

        model->give_dependencies(
            cmd_pool,
            tex_man,
            layout_per_actor,
            layout_per_material,
            sampler,
            graphics_queue,
            phys_device,
            logi_device
        );

        return model;
    }

    HActor VulkanResourceManager::create_actor(
        DescAllocator& desc_allocator,
        const DescLayout_PerActor& desc_layout,
        VkPhysicalDevice phys_device,
        VkDevice logi_device
    ) {
        this->m_actors.push_back(std::make_shared<ActorProxy>());
        auto& actor = this->m_actors.back();
        actor->give_dependencies(desc_allocator, desc_layout, phys_device, logi_device);
        return actor;
    }

    HActorSkinned VulkanResourceManager::create_actor_skinned(
        DescAllocator& desc_allocator,
        const DescLayout_ActorAnimated& desc_layout,
        VkPhysicalDevice phys_device,
        VkDevice logi_device
    ) {
        this->m_skinned_actors.push_back(std::make_shared<ActorSkinnedProxy>());
        auto& actor = this->m_skinned_actors.back();
        actor->give_dependencies(desc_allocator, desc_layout, phys_device, logi_device);
        return actor;
    }

}


// RenderListVK
namespace dal {

    void RenderListVK::apply(dal::Scene& scene, const glm::vec3& view_pos) {
        {
            auto view = scene.m_registry.view<cpnt::ActorStatic>();

            view.each([this](cpnt::ActorStatic& actor) {
                if (!actor.m_model->is_ready()) {
                    return;
                }

                auto& dst_opaque = this->get_render_pair(actor.m_model);
                dst_opaque.m_actors.push_back(&handle_cast(actor.m_actor));
                this->m_used_actors.emplace(actor.m_actor);
            });
        }

        {
            auto view = scene.m_registry.view<cpnt::ActorAnimated>();

            view.each([this](cpnt::ActorAnimated& actor) {
                if (!actor.m_model->is_ready()) {
                    return;
                }

                auto& dst_opaque = this->get_render_pair(actor.m_model);
                dst_opaque.m_actors.push_back(&handle_cast(actor.m_actor));
                this->m_used_skin_actors.emplace(actor.m_actor);
            });
        }

        for (const auto& pair : this->m_static_models) {
            for (const auto actor : pair.m_actors) {
                const auto actor_transform = actor->m_transform.make_mat4();

                for (const auto& unit : pair.m_model->render_units_alpha()) {
                    const auto unit_world_pos = actor_transform * glm::vec4(unit.m_weight_center, 1);
                    const auto to_view = view_pos - glm::vec3(unit_world_pos);

                    auto& dst = this->m_static_alpha_models.emplace_back();
                    dst.m_unit = &unit;
                    dst.m_actor = actor;
                    dst.m_distance_sqr = glm::dot(to_view, to_view);
                }
            }
        }

        for (const auto& pair : this->m_skinned_models) {
            for (const auto actor : pair.m_actors) {
                const auto actor_transform = actor->m_transform.make_mat4();

                for (const auto& unit : pair.m_model->render_units_alpha()) {
                    const auto unit_world_pos = actor_transform * glm::vec4(unit.m_weight_center, 1);
                    const auto to_view = view_pos - glm::vec3(unit_world_pos);

                    auto& dst = this->m_skinned_alpha_models.emplace_back();
                    dst.m_unit = &unit;
                    dst.m_actor = actor;
                    dst.m_distance_sqr = glm::dot(to_view, to_view);
                }
            }
        }

        std::sort(this->m_static_alpha_models.begin(), this->m_static_alpha_models.end());
        std::sort(this->m_skinned_alpha_models.begin(), this->m_skinned_alpha_models.end());

        this->m_plights = scene.m_plights;
        this->m_slights = scene.m_slights;
        this->m_dlight = scene.m_selected_dlight;
        this->m_ambient_light = scene.m_ambient_light;

        // Mirrors
        for (auto& mirror : scene.m_mirrors) {
            auto& mirror_mesh = handle_cast(mirror.m_mesh);
            auto& dst = this->m_render_planes.emplace_back();

            const auto model_mat = mirror.m_actor->m_transform.make_mat4();
            const auto plane = mirror.m_plane.transform(model_mat);

            dst.m_mesh = &mirror_mesh.get();
            dst.m_model_mat = model_mat;
            dst.m_orient_mat = plane.make_reflect_mat();
            dst.m_clip_plane = plane.coeff();
        }

        // Portal pair
        /*for (auto& ppair : scene.m_portal_pairs) {
            auto& p1 = ppair.m_portals[0];
            auto& p2 = ppair.m_portals[1];

            {
                auto& plane1 = this->m_render_planes.emplace_back();
                plane1.m_polygon.push_back({
                    p1.m_vertices[0],
                    p1.m_vertices[1],
                    p1.m_vertices[2],
                });
                plane1.m_polygon.push_back({
                    p1.m_vertices[0],
                    p1.m_vertices[2],
                    p1.m_vertices[3],
                });
                plane1.m_orient_mat = dal::make_portal_mat(p1.m_plane, p2.m_plane);
                plane1.m_clip_plane = p2.m_plane.coeff();
            }

            {
                auto& plane1 = this->m_render_planes.emplace_back();
                plane1.m_polygon.push_back({
                    p2.m_vertices[0],
                    p2.m_vertices[1],
                    p2.m_vertices[2],
                });
                plane1.m_polygon.push_back({
                    p2.m_vertices[0],
                    p2.m_vertices[2],
                    p2.m_vertices[3],
                });
                plane1.m_orient_mat = dal::make_portal_mat(p2.m_plane, p1.m_plane);
                plane1.m_clip_plane = p1.m_plane.coeff();
            }
        }*/

        // Horizontal water
        for (auto& water : scene.m_water_planes) {
            auto& one = this->m_render_waters.emplace_back();

            one.m_mesh = &handle_cast(water.m_mesh).get();
            one.m_model_mat = glm::mat4{1};
            one.m_orient_mat = water.m_plane.make_reflect_mat();
            one.m_clip_plane = water.m_plane.coeff();
        }
    }

    RenderListVK::RenderPair_O_S& RenderListVK::get_render_pair(HRenModel& h_model) {
        auto& model = dal::handle_cast(h_model);

        for (auto& x : this->m_static_models) {
            if (x.m_model == &model.get()) {
                return x;
            }
        }

        this->m_used_models.emplace(h_model);
        auto& output = this->m_static_models.emplace_back();
        output.m_model = &model.get();
        return output;
    }

    RenderListVK::RenderPair_O_A& RenderListVK::get_render_pair(HRenModelSkinned& h_model) {
        auto& model = dal::handle_cast(h_model);

        for (auto& x : this->m_skinned_models) {
            if (x.m_model == &model.get()) {
                return x;
            }
        }

        this->m_used_skin_models.emplace(h_model);
        auto& output = this->m_skinned_models.emplace_back();
        output.m_model = &model.get();
        return output;
    }

}


// Record commands
namespace dal {

    void record_cmd_gbuf(
        const VkCommandBuffer cmd_buf,

        const dal::RenderListVK& render_list,
        const dal::FrameInFlightIndex& flight_frame_index,
        const glm::mat4& proj_view_mat,
        const dal::PlanarReflectionManager& reflection_mgr,

        const VkExtent2D& swapchain_extent,
        const VkDescriptorSet desc_set_per_frame,
        const VkDescriptorSet desc_set_composition,
        const dal::ShaderPipeline& pipeline_gbuf,
        const dal::ShaderPipeline& pipeline_gbuf_animated,
        const dal::ShaderPipeline& pipeline_composition,
        const dal::ShaderPipeline& pipeline_mirror,
        const dal::Fbuf_Gbuf& fbuf,
        const dal::RenderPass_Gbuf& render_pass
    ) {
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
        render_pass_info.framebuffer = fbuf.get();
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
                for (auto& unit : render_pair.m_model->render_units()) {
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

                    for (auto& actor : render_pair.m_actors) {
                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            2,
                            1, &actor->desc_set_at(flight_frame_index),
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
                for (auto& unit : render_pair.m_model->render_units()) {
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

                    for (auto& actor : render_pair.m_actors) {
                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            2,
                            1, &actor->desc_set_at(flight_frame_index),
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

        // Mirror
        {
            auto& pipeline = pipeline_mirror;

            vkCmdNextSubpass(cmd_buf, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& render_plane : render_list.m_render_planes) {
                std::array<VkBuffer, 1> vert_bufs{ render_plane.m_mesh->vertex_buffer() };
                vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                vkCmdBindIndexBuffer(cmd_buf, render_plane.m_mesh->index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.layout(),
                    0,
                    1, &(reflection_mgr.reflection_planes().at(render_plane.reflection_map_index).m_desc.get()),
                    0, nullptr
                );

                U_PC_Mirror pc_data;
                pc_data.m_model_mat = render_plane.m_model_mat;
                pc_data.m_proj_view_mat = proj_view_mat;

                vkCmdPushConstants(
                    cmd_buf,
                    pipeline.layout(),
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(U_PC_Mirror),
                    &pc_data
                );

                vkCmdDrawIndexed(cmd_buf, render_plane.m_mesh->index_size(), 1, 0, 0, 0);
            }

            for (auto& render_plane : render_list.m_render_waters) {
                std::array<VkBuffer, 1> vert_bufs{ render_plane.m_mesh->vertex_buffer() };
                vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                vkCmdBindIndexBuffer(cmd_buf, render_plane.m_mesh->index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(
                    cmd_buf,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.layout(),
                    0,
                    1, &(reflection_mgr.reflection_planes().at(render_plane.reflection_map_index).m_desc.get()),
                    0, nullptr
                );

                U_PC_Mirror pc_data;
                pc_data.m_model_mat = glm::mat4{1};
                pc_data.m_proj_view_mat = proj_view_mat;

                vkCmdPushConstants(
                    cmd_buf,
                    pipeline.layout(),
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(U_PC_Mirror),
                    &pc_data
                );

                vkCmdDrawIndexed(cmd_buf, render_plane.m_mesh->index_size(), 1, 0, 0, 0);
            }
        }

        vkCmdEndRenderPass(cmd_buf);
        if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS)
            dalAbort("failed to record command buffer!");
    }

    void record_cmd_final(
        const VkCommandBuffer cmd_buf,

        const VkExtent2D& extent,
        const VkDescriptorSet desc_set_final,
        const dal::ShaderPipeline& pipeline_final,
        const dal::Fbuf_Final& fbuf,
        const dal::RenderPass_Final& renderpass
    ) {
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
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_final.pipeline());

            vkCmdBindDescriptorSets(
                cmd_buf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_final.layout(),
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

    void record_cmd_alpha(
        const VkCommandBuffer cmd_buf,

        const dal::RenderListVK& render_list,
        const dal::FrameInFlightIndex& flight_frame_index,
        const glm::vec3& view_pos,

        const VkExtent2D& swapchain_extent,
        const VkDescriptorSet desc_set_per_global,
        const VkDescriptorSet desc_set_composition,
        const dal::ShaderPipeline& pipeline_alpha,
        const dal::ShaderPipeline& pipeline_alpha_animated,
        const dal::Fbuf_Alpha& fbuf,
        const dal::RenderPass_Alpha& render_pass
    ) {
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
        render_pass_info.framebuffer = fbuf.get();
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

            for (auto& render_tuple : render_list.m_static_alpha_models) {
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
                    1, &render_tuple.m_actor->desc_set_at(flight_frame_index),
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

            for (auto& render_tuple : render_list.m_skinned_alpha_models) {
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
                    1, &render_tuple.m_actor->desc_set_at(flight_frame_index),
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

    void record_cmd_shadow(
        const VkCommandBuffer cmd_buf,

        const dal::RenderListVK& render_list,
        const dal::FrameInFlightIndex& flight_frame_index,
        const glm::mat4& light_mat,

        const VkExtent2D& shadow_map_extent,
        const dal::ShaderPipeline& pipeline_shadow,
        const dal::ShaderPipeline& pipeline_shadow_animated,
        const dal::Fbuf_Shadow& fbuf,
        const dal::RenderPass_ShadowMap& render_pass
    ) {
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
        render_pass_info.framebuffer = fbuf.get();
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = shadow_map_extent;
        render_pass_info.clearValueCount = clear_colors.size();
        render_pass_info.pClearValues = clear_colors.data();

        vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        {
            auto& pipeline = pipeline_shadow;
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            const auto [viewport, scissor] = ::create_info_viewport_scissor(shadow_map_extent);
            vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
            vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& render_tuple : render_list.m_static_models) {
                auto& model = *render_tuple.m_model;

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

            const auto [viewport, scissor] = ::create_info_viewport_scissor(shadow_map_extent);
            vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
            vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& render_tuple : render_list.m_skinned_models) {
                auto& model = *render_tuple.m_model;

                for (auto& unit : model.render_units()) {
                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    for (auto& actor : render_tuple.m_actors) {
                        U_PC_Shadow pc_data;
                        pc_data.m_model_mat = actor->m_transform.make_mat4();
                        pc_data.m_light_mat = light_mat;
                        vkCmdPushConstants(cmd_buf, pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(U_PC_Shadow), &pc_data);

                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            0,
                            1, &actor->desc_set_at(flight_frame_index),
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

    void record_cmd_on_mirror(
        const VkCommandBuffer cmd_buf,

        const dal::RenderListVK& render_list,
        const dal::FrameInFlightIndex& flight_frame_index,

        dal::U_PC_OnMirror push_constant,
        const VkExtent2D& extent,
        const dal::ShaderPipeline& pipeline_on_mirror,
        const dal::ShaderPipeline& pipeline_on_mirror_animated,
        const dal::Fbuf_Simple& fbuf,
        const dal::RenderPass_Simple& render_pass
    ) {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if (VK_SUCCESS != vkBeginCommandBuffer(cmd_buf, &begin_info))
            dalAbort("failed to begin recording command buffer!");

        std::array<VkClearValue, 2> clear_colors{};
        clear_colors[0].color = { 0, 0, 0, 1 };
        clear_colors[1].depthStencil = { 1, 0 };

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass.get();
        render_pass_info.framebuffer = fbuf.get();
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = extent;
        render_pass_info.clearValueCount = clear_colors.size();
        render_pass_info.pClearValues = clear_colors.data();

        vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        {
            auto& pipeline = pipeline_on_mirror;
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
            vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
            vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

            vkCmdPushConstants(cmd_buf, pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(U_PC_OnMirror), &push_constant);

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& render_tuple : render_list.m_static_models) {
                auto& model = *render_tuple.m_model;

                for (auto& unit : model.render_units()) {
                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline.layout(),
                        0,
                        1, &unit.m_material.m_descset.get(),
                        0, nullptr
                    );

                    for (auto& actor : render_tuple.m_actors) {
                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            1,
                            1, &actor->desc_set_at(flight_frame_index),
                            0, nullptr
                        );

                        vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                    }
                }
            }
        }

        {
            auto& pipeline = pipeline_on_mirror_animated;
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());

            const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
            vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
            vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

            vkCmdPushConstants(cmd_buf, pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(U_PC_OnMirror), &push_constant);

            std::array<VkDeviceSize, 1> vert_offsets{ 0 };

            for (auto& render_tuple : render_list.m_skinned_models) {
                auto& model = *render_tuple.m_model;

                for (auto& unit : model.render_units()) {
                    std::array<VkBuffer, 1> vert_bufs{ unit.m_vert_buffer.vertex_buffer() };
                    vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                    vkCmdBindIndexBuffer(cmd_buf, unit.m_vert_buffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(
                        cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline.layout(),
                        0,
                        1, &unit.m_material.m_descset.get(),
                        0, nullptr
                    );

                    for (auto& actor : render_tuple.m_actors) {
                        vkCmdBindDescriptorSets(
                            cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(),
                            1,
                            1, &actor->desc_set_at(flight_frame_index),
                            0, nullptr
                        );

                        vkCmdDrawIndexed(cmd_buf, unit.m_vert_buffer.index_size(), 1, 0, 0, 0);
                    }
                }
            }
        }

        vkCmdEndRenderPass(cmd_buf);

        if (VK_SUCCESS != vkEndCommandBuffer(cmd_buf))
            dalAbort("failed to record command buffer!");
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

}


// FbufManager
namespace dal {

    void FbufManager::init(
        const std::vector<dal::ImageView>& swapchain_views,
        const dal::AttachmentBundle_Gbuf& attach_man,
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

}


// UbufManager
namespace dal {

    void UbufManager::init(
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_ub_glights.init(
            MAX_FRAMES_IN_FLIGHT,
            phys_device,
            logi_device
        );

        this->m_u_camera_transform.init(
            MAX_FRAMES_IN_FLIGHT,
            phys_device,
            logi_device
        );

        this->m_ub_final.init(phys_device, logi_device);
    }

    void UbufManager::destroy(const VkDevice logi_device) {
        this->m_ub_glights.destroy(logi_device);
        this->m_u_camera_transform.destroy(logi_device);
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
        RenderListVK render_list;
        FrameInFlightIndex index0{0};
        std::array<VkPipelineStageFlags, 0> wait_stages{};
        std::array<VkSemaphore, 0> wait_semaphores{};
        std::array<VkSemaphore, 0> signal_semaphores{};

        for (size_t i = 0; i < this->m_dlights.size(); ++i) {
            auto& shadow_map = this->m_dlights[i];
            auto& cmd_buf = shadow_map.cmd_buf_at(index0.get());

            record_cmd_shadow(
                cmd_buf,
                render_list,
                index0,
                glm::mat4{1},
                shadow_map.extent(),
                pipelines.shadow(),
                pipelines.shadow_animated(),
                shadow_map.fbuf(),
                render_pass
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
                logi_device.queue_graphics(),
                1,
                &submit_info,
                VK_NULL_HANDLE
            );

            dalAssert(VK_SUCCESS == submit_result);
        }

        for (size_t i = 0; i < this->m_slights.size(); ++i) {
            auto& shadow_map = this->m_slights[i];
            auto& cmd_buf = shadow_map.cmd_buf_at(index0.get());

            record_cmd_shadow(
                cmd_buf,
                render_list,
                index0,
                glm::mat4{1},
                shadow_map.extent(),
                pipelines.shadow(),
                pipelines.shadow_animated(),
                shadow_map.fbuf(),
                render_pass
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


// ReflectionPlane
namespace dal {

    void ReflectionPlane::init(
        const uint32_t width,
        const uint32_t height,
        const uint32_t max_in_flight_count,
        CommandPool& cmd_pool,
        DescPool& desc_pool,
        const dal::SamplerTexture& sampler,
        const dal::DescLayout_Mirror& desc_layout,
        const dal::RenderPass_Simple& renderpass,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(cmd_pool, desc_pool, logi_device);

        this->m_attachments.init(
            width, height,
            renderpass,
            phys_device,
            logi_device
        );

        this->m_fbuf.init(
            renderpass,
            VkExtent2D{ width, height },
            this->m_attachments.color().view().get(),
            this->m_attachments.depth().view().get(),
            logi_device
        );

        this->m_cmd_buf = cmd_pool.allocate(max_in_flight_count, logi_device);
        this->m_desc = desc_pool.allocate(desc_layout, logi_device);
        this->m_desc.record_mirror(this->m_attachments.color().view().get(), sampler, logi_device);
    }

    void ReflectionPlane::destroy(CommandPool& cmd_pool, DescPool& desc_pool, const VkDevice logi_device) {
        if (!this->m_cmd_buf.empty()) {
            cmd_pool.free(this->m_cmd_buf, logi_device);
            this->m_cmd_buf.clear();
        }

        this->m_fbuf.destroy(logi_device);
        this->m_attachments.destroy(logi_device);
    }

    bool ReflectionPlane::is_ready() const {
        return (
            this->m_fbuf.is_ready() &&
            this->m_desc.is_ready()
        );
    }

}


// PlanarReflectionManager
namespace dal {

    void PlanarReflectionManager::init(
        const VkPhysicalDevice phys_device,
        const LogicalDevice& logi_device
    ) {
        this->destroy(logi_device.get());

        this->m_sampler.init(false, phys_device, logi_device.get());
        this->m_cmd_pool.init(logi_device.indices().graphics_family(), logi_device.get());
        this->m_desc_pool.init(10, 10, 10, 10, logi_device.get());
    }

    void PlanarReflectionManager::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_planes)
            x.destroy(this->m_cmd_pool, this->m_desc_pool, logi_device);
        this->m_planes.clear();

        this->m_sampler.destroy(logi_device);
        this->m_cmd_pool.destroy(logi_device);
        this->m_desc_pool.destroy(logi_device);
    }

    ReflectionPlane& PlanarReflectionManager::new_plane(
        const uint32_t width,
        const uint32_t height,
        const dal::DescLayout_Mirror& desc_layout,
        const dal::RenderPass_Simple& renderpass,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        auto& plane = this->m_planes.emplace_back();

        plane.init(
            width,
            height,
            dal::MAX_FRAMES_IN_FLIGHT,
            this->m_cmd_pool,
            this->m_desc_pool,
            this->m_sampler,
            desc_layout,
            renderpass,
            phys_device,
            logi_device
        );

        return plane;
    }

    void PlanarReflectionManager::resize(
        const size_t size,
        const uint32_t width,
        const uint32_t height,
        const dal::DescLayout_Mirror& desc_layout,
        const dal::RenderPass_Simple& renderpass,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        if (size == this->m_planes.size()) {
            return;
        }
        else if (size > this->m_planes.size()) {
            const auto needed_count = size - this->m_planes.size();

            for (size_t i = 0; i < needed_count; ++i) {
                this->new_plane(
                    width, height,
                    desc_layout,
                    renderpass,
                    phys_device,
                    logi_device
                );
            }

            dalInfo(fmt::format("Created {} reflection planes", needed_count).c_str());
        }
        else {
            const auto destroy_count = this->m_planes.size() - size;

            for (size_t i = 0; i < destroy_count; ++i) {
                this->m_planes.back().destroy(this->m_cmd_pool, this->m_desc_pool, logi_device);
                this->m_planes.pop_back();
            }

            dalInfo(fmt::format("Destroyed {} reflection planes", destroy_count).c_str());
        }
    }

}
