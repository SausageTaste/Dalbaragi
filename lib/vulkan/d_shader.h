#pragma once

#include <utility>

#include "d_filesystem.h"
#include "d_render_pass.h"


namespace dal {

    class ShaderPipeline {

    private:
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;

    public:
        ShaderPipeline() = default;
        ShaderPipeline(const ShaderPipeline&) = delete;
        ShaderPipeline& operator=(const ShaderPipeline&) = delete;

    public:
        ShaderPipeline(const VkPipeline pipeline, const VkPipelineLayout layout, const VkDevice logi_device) {
            this->init(pipeline, layout, logi_device);
        }

        ShaderPipeline(ShaderPipeline&&) noexcept;

        ShaderPipeline& operator=(ShaderPipeline&&) noexcept;

        void init(const VkPipeline pipeline, const VkPipelineLayout layout, const VkDevice logi_device);

        void init(const std::pair<VkPipeline, VkPipelineLayout>& pipeline, const VkDevice logi_device) {
            this->init(pipeline.first, pipeline.second, logi_device);
        }

        void destroy(const VkDevice logi_device);

        auto& pipeline() const {
            return this->m_pipeline;
        }

        auto& layout() const {
            return this->m_layout;
        }

    };


    class PipelineManager {

    private:
        ShaderPipeline m_gbuf;
        ShaderPipeline m_composition;
        ShaderPipeline m_final;
        ShaderPipeline m_alpha;

    public:
        void init(
            dal::filesystem::AssetManager& asset_mgr,
            const bool need_gamma_correction,
            const VkExtent2D& swapchain_extent,
            const VkExtent2D& gbuf_extent,
            const VkDescriptorSetLayout desc_layout_final,
            const VkDescriptorSetLayout desc_layout_simple,
            const VkDescriptorSetLayout desc_layout_per_material,
            const VkDescriptorSetLayout desc_layout_per_actor,
            const VkDescriptorSetLayout desc_layout_per_world,
            const VkDescriptorSetLayout desc_layout_composition,
            const RenderPass_Gbuf& rp_gbuf,
            const RenderPass_Final& rp_final,
            const RenderPass_Alpha& rp_alpha,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& simple() const {
            return this->m_gbuf;
        }

        auto& composition() const {
            return this->m_composition;
        }

        auto& final() const {
            return this->m_final;
        }

        auto& alpha() const {
            return this->m_alpha;
        }

    };

}
