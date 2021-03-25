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

    public:
        ShaderPipeline m_simple;

    public:
        void init(
            dal::filesystem::AssetManager& asset_mgr,
            const VkExtent2D& swapchain_extent,
            const VkDescriptorSetLayout* const desc_set_layouts, const uint32_t desc_set_layout_count,
            const VkRenderPass renderpass,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& get_simple() const {
            return this->m_simple;
        }

    };

}
