#pragma once

#include <vector>

#include "d_konsts.h"
#include "d_shader.h"
#include "d_indices.h"
#include "d_command.h"
#include "d_vk_device.h"
#include "d_swapchain.h"
#include "d_render_pass.h"
#include "d_framebuffer.h"
#include "d_model_renderer.h"


namespace dal {

    inline auto& actor_cast(dal::IActor& actor) {
        return dynamic_cast<dal::ActorVK&>(actor);
    }

    inline auto& actor_cast(const dal::IActor& actor) {
        return dynamic_cast<const dal::ActorVK&>(actor);
    }

    inline auto& actor_cast(dal::HActor& actor) {
        return dynamic_cast<dal::ActorVK&>(*actor.get());
    }

    inline auto& actor_cast(const dal::HActor& actor) {
        return dynamic_cast<const dal::ActorVK&>(*actor.get());
    }

    inline auto& actor_cast(dal::IActorSkinned& actor) {
        return dynamic_cast<dal::ActorSkinnedVK&>(actor);
    }

    inline auto& actor_cast(const dal::IActorSkinned& actor) {
        return dynamic_cast<const dal::ActorSkinnedVK&>(actor);
    }

    inline auto& actor_cast(dal::HActorSkinned& actor) {
        return dynamic_cast<dal::ActorSkinnedVK&>(*actor.get());
    }

    inline auto& actor_cast(const dal::HActorSkinned& actor) {
        return dynamic_cast<const dal::ActorSkinnedVK&>(*actor.get());
    }

    inline auto& model_cast(dal::IRenModel& model) {
        return dynamic_cast<dal::ModelRenderer&>(model);
    }

    inline auto& model_cast(const dal::IRenModel& model) {
        return dynamic_cast<const dal::ModelRenderer&>(model);
    }

    inline auto& model_cast(dal::HRenModel& model) {
        return dynamic_cast<dal::ModelRenderer&>(*model);
    }

    inline auto& model_cast(const dal::HRenModel& model) {
        return dynamic_cast<const dal::ModelRenderer&>(*model);
    }

    inline auto& model_cast(dal::IRenModelSkineed& model) {
        return dynamic_cast<dal::ModelSkinnedRenderer&>(model);
    }

    inline auto& model_cast(const dal::IRenModelSkineed& model) {
        return dynamic_cast<const dal::ModelSkinnedRenderer&>(model);
    }

    inline auto& model_cast(dal::HRenModelSkinned& model) {
        return dynamic_cast<dal::ModelSkinnedRenderer&>(*model);
    }

    inline auto& model_cast(const dal::HRenModelSkinned& model) {
        return dynamic_cast<const dal::ModelSkinnedRenderer&>(*model);
    }


    class RenderListVK {

    private:
        template <typename _Model, typename _Actor>
        struct RenderPairOpaqueVK {
            std::vector<const _Actor*> m_actors;
            const _Model* m_model = nullptr;
        };

        template <typename _Actor>
        struct RenderPairTranspVK {
            const _Actor* m_actor = nullptr;
            const RenderUnit* m_unit = nullptr;
            float m_distance_sqr = 0;

            bool operator<(const RenderPairTranspVK& other) const {
                return this->m_distance_sqr > other.m_distance_sqr;
            }
        };

        // O : Opaque, A : Alpha
        // S : Static, A : Animated
        using RenderPair_O_S = RenderPairOpaqueVK<ModelRenderer,        ActorVK       >;
        using RenderPair_O_A = RenderPairOpaqueVK<ModelSkinnedRenderer, ActorSkinnedVK>;
        using RenderPair_A_S = RenderPairTranspVK<ActorVK       >;
        using RenderPair_A_A = RenderPairTranspVK<ActorSkinnedVK>;

    public:
        std::vector<RenderPair_O_S> m_static_models;
        std::vector<RenderPair_A_S> m_static_alpha_models;
        std::vector<RenderPair_O_A> m_skinned_models;
        std::vector<RenderPair_A_A> m_skinned_alpha_models;

    public:
        void apply(const dal::RenderList& render_list, const glm::vec3& view_pos);

    };



    class CmdPoolManager {

    private:
        CommandPool m_general_pool;
        std::vector<CommandPool> m_pools;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_simple;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_final;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_alpha;  // Per swapchain

    public:
        void init(const uint32_t swapchain_count, const uint32_t queue_family_index, const VkDevice logi_device);

        void destroy(const VkDevice logi_device);

        void record_simple(
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

        auto& general_pool() {
            return this->m_general_pool;
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
        UniformBuffer<U_PerFrame_InFinal> m_ub_final;

    public:
        void init(
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

    };


    class ShadowMap {

    private:
        FbufAttachment m_depth_attach;
        Fbuf_Shadow m_fbuf;
        std::vector<VkCommandBuffer> m_cmd_buf;

    public:
        void init(
            const uint32_t width,
            const uint32_t height,
            const uint32_t max_in_flight_count,
            CommandPool& cmd_pool,
            const RenderPass_ShadowMap& rp_shadow,
            const VkFormat depth_format,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(CommandPool& cmd_pool, const VkDevice logi_device);

        void record_cmd_buf(
            const FrameInFlightIndex& flight_frame_index,
            const RenderList& render_list,
            const glm::mat4& light_mat,
            const ShaderPipeline& pipeline_shadow,
            const ShaderPipeline& pipeline_shadow_animated,
            const RenderPass_ShadowMap& render_pass
        );

        auto& cmd_buf_at(const size_t index) const {
            return this->m_cmd_buf.at(index);
        }

        auto shadow_map_view() const {
            return this->m_depth_attach.view().get();
        }

    };


    class ShadowMapManager {

    public:
        std::array<ShadowMap, dal::MAX_DLIGHT_COUNT> m_dlights;
        std::array<glm::mat4, dal::MAX_DLIGHT_COUNT> m_dlight_matrices;
        std::array<float, dal::MAX_DLIGHT_COUNT> m_dlight_clip_dists;
        std::array<dal::Timer, dal::MAX_DLIGHT_COUNT> m_dlight_timers;

        std::array<ShadowMap, dal::MAX_SLIGHT_COUNT> m_slights;

    private:
        CommandPool m_cmd_pool;
        std::array<VkImageView, dal::MAX_DLIGHT_COUNT> m_dlight_views;
        std::array<VkImageView, dal::MAX_SLIGHT_COUNT> m_slight_views;

    public:
        void init(
            const RenderPass_ShadowMap& renderpass,
            const PhysicalDevice& phys_device,
            const LogicalDevice& logi_device
        );

        void destroy(const VkDevice logi_device);

        void render_empty_for_all(
            const PipelineManager& pipelines,
            const RenderPass_ShadowMap& render_pass,
            const LogicalDevice& logi_device
        );

        auto& dlight_views() const {
            return this->m_dlight_views;
        }

        auto& slight_views() const {
            return this->m_slight_views;
        }

        std::array<bool, dal::MAX_DLIGHT_COUNT> create_dlight_update_flags();

    };

}
