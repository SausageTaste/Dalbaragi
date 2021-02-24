#include "d_shader.h"

#include "d_logger.h"
#include "d_filesystem.h"


// Shader module tools
namespace {

    class ShaderModule {

    private:
        const VkDevice m_parent_device;
        VkShaderModule m_module = VK_NULL_HANDLE;

    public:
        ShaderModule(const VkDevice logi_device)
            : m_parent_device(logi_device)
        {

        }

        ShaderModule(const VkDevice logi_device, const char* const source_str, const uint32_t source_size)
            : m_parent_device(logi_device)
        {
            this->init(source_str, source_size);
        }

        ~ShaderModule() {
            this->destroy();
        }

        void init(const char* const source_str, const uint32_t source_size) {
            this->destroy();

            VkShaderModuleCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = source_size;
            create_info.pCode = reinterpret_cast<const uint32_t*>(source_str);

            if (VK_SUCCESS != vkCreateShaderModule(this->m_parent_device, &create_info, nullptr, &this->m_module)) {
                dalAbort("Failed to create shader module");
            }
        }

        void destroy() {
            if (VK_NULL_HANDLE != this->m_module) {
                vkDestroyShaderModule(this->m_parent_device, this->m_module, nullptr);
                this->m_module = VK_NULL_HANDLE;
            }
        }

        VkShaderModule get() const {
            return this->m_module;
        }

        VkShaderModule operator*() const {
            return this->get();
        }

    };

}


// ShaderPipeline
namespace dal {

    ShaderPipeline::ShaderPipeline(ShaderPipeline&& other) noexcept {
        std::swap(this->m_pipeline, other.m_pipeline);
        std::swap(this->m_layout, other.m_layout);
    }

    ShaderPipeline& ShaderPipeline::operator=(ShaderPipeline&& other) noexcept {
        std::swap(this->m_pipeline, other.m_pipeline);
        std::swap(this->m_layout, other.m_layout);

        return *this;
    }

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

    auto create_info_shader_stage(const ShaderModule& vert_shader_module, const ShaderModule& frag_shader_module) {
        std::array<VkPipelineShaderStageCreateInfo, 2> result{};

        result[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        result[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        result[0].module = vert_shader_module.get();
        result[0].pName = "main";

        result[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        result[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        result[1].module = frag_shader_module.get();
        result[1].pName = "main";

        return result;
    }


    dal::ShaderPipeline make_pipeline_simple(const VkDevice logi_device) {
        const auto vert_src = dal::filesystem::asset::open("shader/simple_v.spv")->read_stl<std::vector<char>>();
        if (!vert_src) {
            dalAbort("Vertex shader 'simple_v.spv' not found");
        }
        const auto frag_src = dal::filesystem::asset::open("shader/simple_f.spv")->read_stl<std::vector<char>>();
        if (!frag_src) {
            dalAbort("Fragment shader 'simple_f.spv' not found");
        }

        const ShaderModule vert_shader_module(logi_device, vert_src->data(), vert_src->size());
        const ShaderModule frag_shader_module(logi_device, frag_src->data(), frag_src->size());
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        dal::ShaderPipeline result{};
        return result;
    }

}


// PipelineManager
namespace dal {

    void PipelineManager::init(const VkDevice logi_device) {
        this->destroy(logi_device);

        this->m_simple = ::make_pipeline_simple(logi_device);
    }

    void PipelineManager::destroy(const VkDevice logi_device) {
        this->m_simple.destroy(logi_device);
    }

}
