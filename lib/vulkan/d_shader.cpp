#include "d_shader.h"

#include "d_logger.h"
#include "d_filesystem.h"


// ShaderPipeline
namespace dal {

    void ShaderPipeline::init(const VkPipeline pipeline, const VkPipelineLayout layout, const VkDevice logi_device) {
        this->destroy(logi_device);

        this->m_pipeline = pipeline;
        this->m_layout = layout;
    }

    void ShaderPipeline::init(const std::pair<VkPipeline, VkPipelineLayout>& pipeline, const VkDevice logi_device) {
        this->init(pipeline.first, pipeline.second, logi_device);
    }

    void ShaderPipeline::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_layout) {
            vkDestroyPipelineLayout(logi_device, this->m_layout, nullptr);
            this->m_layout = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_pipeline) {
            vkDestroyPipeline(logi_device, this->m_pipeline, nullptr);
            this->m_pipeline = VK_NULL_HANDLE;
        }
    }

}


// Pipeline creating functions
namespace {

    std::pair<VkPipeline, VkPipelineLayout> make_pipeline_simple() {
        const auto vert_src = dal::filesystem::asset::open("shader/simple_v.spv")->read_stl<std::vector<char>>();
        const auto frag_src = dal::filesystem::asset::open("shader/simple_f.spv")->read_stl<std::vector<char>>();

        return { VK_NULL_HANDLE, VK_NULL_HANDLE };
    }

}


// PipelineManager
namespace dal {

    void PipelineManager::init(const VkDevice logi_device) {
        this->destroy(logi_device);

        this->m_simple.init(::make_pipeline_simple(), logi_device);
    }

    void PipelineManager::destroy(const VkDevice logi_device) {
        this->m_simple.destroy(logi_device);
    }

}
