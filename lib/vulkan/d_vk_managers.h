#pragma once

#include <vector>

#include "d_konsts.h"
#include "d_shader.h"
#include "d_indices.h"
#include "d_command.h"
#include "d_geometry.h"
#include "d_vk_device.h"
#include "d_swapchain.h"
#include "d_render_pass.h"
#include "d_framebuffer.h"
#include "d_model_renderer.h"


namespace dal {

    class PlanarReflectionManager;


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

        struct Triangle {
            std::array<glm::vec3, 3> m_vertices;
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

        std::array<Triangle, 2> m_mirror_mesh;

    public:
        void apply(const dal::RenderList& render_list, const glm::vec3& view_pos);

    };


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
    );

    void record_cmd_final(
        const VkCommandBuffer cmd_buf,

        const VkExtent2D& extent,
        const VkDescriptorSet desc_set_final,
        const dal::ShaderPipeline& pipeline_final,
        const dal::Fbuf_Final& fbuf,
        const dal::RenderPass_Final& renderpass
    );

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
    );

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
    );

    void record_cmd_simple(
        const VkCommandBuffer cmd_buf,

        const dal::RenderListVK& render_list,
        const dal::FrameInFlightIndex& flight_frame_index,

        dal::U_PC_OnMirror push_constant,
        const VkExtent2D& extent,
        const dal::ShaderPipeline& pipeline_simple,
        const dal::Fbuf_Simple& fbuf,
        const dal::RenderPass_Simple& render_pass
    );


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
        std::vector<dal::Fbuf_Gbuf> m_fbuf_simple;
        std::vector<dal::Fbuf_Final> m_fbuf_final;
        std::vector<dal::Fbuf_Alpha> m_fbuf_alpha;

    public:
        void init(
            const std::vector<dal::ImageView>& swapchain_views,
            const dal::AttachmentBundle_Gbuf& attach_man,
            const VkExtent2D& swapchain_extent,
            const VkExtent2D& gbuf_extent,
            const dal::RenderPass_Gbuf& rp_gbuf,
            const dal::RenderPass_Final& rp_final,
            const dal::RenderPass_Alpha& rp_alpha,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& fbuf_gbuf_at(const dal::SwapchainIndex index) const {
            return this->m_fbuf_simple.at(*index);
        }

        auto& fbuf_final_at(const dal::SwapchainIndex index) const {
            return this->m_fbuf_final.at(*index);
        }

        auto& fbuf_alpha_at(const dal::SwapchainIndex index) const {
            return this->m_fbuf_alpha.at(*index);
        }

    };


    class UbufManager {

    public:
        UniformBufferArray<U_GlobalLight> m_ub_glights;
        UniformBufferArray<U_CameraTransform> m_u_camera_transform;
        UniformBuffer<U_Shader_Final> m_ub_final;

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

        auto& cmd_buf_at(const size_t index) const {
            return this->m_cmd_buf.at(index);
        }

        auto shadow_map_view() const {
            return this->m_depth_attach.view().get();
        }

        auto extent() const {
            return this->m_depth_attach.extent();
        }

        auto& fbuf() const {
            return this->m_fbuf;
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


    class ReflectionPlane {

    public:
        AttachmentBundle_Simple m_attachments;
        Fbuf_Simple m_fbuf;
        std::vector<VkCommandBuffer> m_cmd_buf;
        DescSet m_desc;
        dal::Plane m_geometry;

    public:
        void init(
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
        );

        void destroy(CommandPool& cmd_pool, DescPool& desc_pool, const VkDevice logi_device);

    };


    class PlanarReflectionManager {

    private:
        std::vector<ReflectionPlane> m_planes;
        SamplerTexture m_sampler;
        CommandPool m_cmd_pool;
        DescPool m_desc_pool;

    public:
        void init(
            const VkPhysicalDevice phys_device,
            const LogicalDevice& logi_device
        );

        void destroy(const VkDevice logi_device);

        ReflectionPlane& new_plane(
            const uint32_t width,
            const uint32_t height,
            const dal::DescLayout_Mirror& desc_layout,
            const dal::RenderPass_Simple& renderpass,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        auto& reflection_planes() {
            return this->m_planes;
        }

        auto& reflection_planes() const {
            return this->m_planes;
        }

    };

}
