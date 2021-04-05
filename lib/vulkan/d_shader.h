#pragma once

#include <utility>

#include "d_vulkan_header.h"
#include "d_filesystem.h"


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
        ShaderPipeline m_simple;

    public:
        void init(
            dal::filesystem::AssetManager& asset_mgr,
            const bool need_gamma_correction,
            const VkExtent2D& swapchain_extent,
            const VkDescriptorSetLayout desc_layout_simple,
            const VkDescriptorSetLayout desc_layout_per_material,
            const VkRenderPass renderpass,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& simple() const {
            return this->m_simple;
        }

    };

}
