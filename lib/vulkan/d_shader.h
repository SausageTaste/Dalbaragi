#pragma once

#include <utility>

#include "d_vulkan_header.h"


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
        ShaderPipeline(ShaderPipeline&&) noexcept;
        ShaderPipeline& operator=(ShaderPipeline&&) noexcept;

        void init(const VkPipeline pipeline, const VkPipelineLayout layout, const VkDevice logi_device);
        void init(const std::pair<VkPipeline, VkPipelineLayout>& pipeline, const VkDevice logi_device);
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
        void init(const VkDevice logi_device);
        void destroy(const VkDevice logi_device);

    };

}
