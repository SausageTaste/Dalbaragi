#include "d_shader.h"

#include <set>
#include <list>
#include <array>

#include <fmt/format.h>
#include <shaderc/shaderc.hpp>

#include "d_logger.h"
#include "d_uniform.h"
#include "d_filesystem.h"
#include "d_vert_data.h"


#define DAL_HASH_SHADER_CACHE_NAME false
#define DAL_INVALIDATE_CACHE_ONCE false


// Shader module tools
namespace {

    const std::filesystem::path SHADER_CACHE_DIR = "_internal/spv";

#if DAL_INVALIDATE_CACHE_ONCE
        std::unordered_set<std::string> g_refreshed_ones;
#endif


    enum class ShaderKind {
        vert,
        frag,
    };


    class ShaderCompileOption {

    private:
        struct ResultData {

        public:
            std::string m_content;
            std::string m_source_name;

        private:
            shaderc_include_result m_result;

            void update_result() noexcept {
                this->m_result.content              = this->m_content.c_str();
                this->m_result.content_length       = this->m_content.size();
                this->m_result.source_name          = this->m_source_name.c_str();
                this->m_result.source_name_length   = this->m_source_name.size();
                this->m_result.user_data            = nullptr;
            }

        public:
            shaderc_include_result& update_get_result() noexcept {
                this->update_result();
                return this->m_result;
            }

        };


        class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {

        private:
            dal::Filesystem& m_filesys;
            std::list<ResultData> m_list_result;

        public:
            ShaderIncluder(dal::Filesystem& filesys)
                : m_filesys(filesys)
            {

            }

            shaderc_include_result* GetInclude(
                const char* const requested_source,
                const shaderc_include_type type,
                const char* const requesting_source,
                const size_t include_depth
            ) override {
                auto& output = this->m_list_result.emplace_back();

                const auto requested_file_path = std::filesystem::path{ requesting_source }.remove_filename() / requested_source;
                output.m_source_name = requested_file_path.string();

                auto file = this->m_filesys.open(output.m_source_name);
                if (!file->is_ready()) {
                    output.m_content = fmt::format("Failed to open shader file: {}", output.m_source_name);
                }
                else {
                    if (!file->read_stl(output.m_content)) {
                        output.m_content = fmt::format("Failed to read shader file: {}", output.m_source_name);
                    }
                }

                return &output.update_get_result();
            }

            void ReleaseInclude(shaderc_include_result* const data) override {

            }

        };


    private:
        shaderc::CompileOptions m_options;
        std::set<std::string> m_macro_def;

        size_t m_hash;

    public:
        ShaderCompileOption(dal::Filesystem& filesys) {
            this->m_options.SetIncluder(std::make_unique<ShaderIncluder>(filesys));
            this->m_options.SetOptimizationLevel(shaderc_optimization_level_performance);

            this->update_hash();
        }

        void add_macro_def(const std::string& def) {
            this->m_macro_def.insert(def);
            this->m_options.AddMacroDefinition(def);

            this->update_hash();
        }

        auto& get() const {
            return this->m_options;
        }

        auto& hash_value() const {
            return this->m_hash;
        }

    private:
        void update_hash() {
            std::string accum;

            for (auto& x : this->m_macro_def) {
                accum += x;
                accum += ' ';
            }

            this->m_hash = std::hash<std::string>{}(accum);
        }

    };


    class ShaderCompiler {

    private:
        shaderc::Compiler m_compiler;

    public:
        template <typename T>
        std::pair<bool, std::string> compile(
            const char* const src_data,
            const size_t src_size,
            const ::ShaderKind shader_kind,
            const char* const src_path,
            const ::ShaderCompileOption& options,
            std::vector<T>& output
        ) const {
            static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1);
            std::pair<bool, std::string> result_output;

            const auto result_data = this->m_compiler.CompileGlslToSpv(
                src_data,
                src_size,
                this->convert_shader_kind(shader_kind),
                src_path,
                options.get()
            );

            if (shaderc_compilation_status_success != result_data.GetCompilationStatus()) {
                result_output.first = false;
                result_output.second = result_data.GetErrorMessage();
            }
            else {
                output.insert(
                    output.end(),
                    reinterpret_cast<const T*>(result_data.cbegin()),
                    reinterpret_cast<const T*>(result_data.cend())
                );

                result_output.first = true;
            }

            return result_output;
        }

    private:
        static shaderc_shader_kind convert_shader_kind(const ::ShaderKind shader_kind) {
            switch (shader_kind) {
                case ::ShaderKind::vert:
                    return shaderc_glsl_default_vertex_shader;
                case ::ShaderKind::frag:
                    return shaderc_glsl_default_fragment_shader;
                default:
                    return shaderc_glsl_infer_from_source;
            }
        }

    };


    class ShaderModule {

    private:
        const VkDevice m_parent_device;
        VkShaderModule m_module = VK_NULL_HANDLE;

    public:
        ShaderModule(const VkDevice logi_device)
            : m_parent_device(logi_device)
        {

        }

        template <typename T>
        ShaderModule(const VkDevice logi_device, const T* const source_str, const size_t source_size)
            : m_parent_device(logi_device)
        {
            this->init(source_str, source_size);
        }

        template <typename T>
        ShaderModule(const VkDevice logi_device, const std::vector<T>& source)
            : m_parent_device(logi_device)
        {
            this->init(source);
        }

        ~ShaderModule() {
            this->destroy();
        }

        template <typename T>
        void init(const T* const source_str, const size_t source_size) {
            this->destroy();

            VkShaderModuleCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = source_size;
            create_info.pCode = reinterpret_cast<const uint32_t*>(source_str);

            if (VK_SUCCESS != vkCreateShaderModule(this->m_parent_device, &create_info, nullptr, &this->m_module)) {
                dalAbort("Failed to create shader module");
            }
        }

        template <typename T>
        void init(const std::vector<T>& source) {
            this->destroy();

            VkShaderModuleCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = source.size() * sizeof(T);
            create_info.pCode = reinterpret_cast<const uint32_t*>(source.data());

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


    class ShaderSrcManager {

    public:
        ::ShaderCompileOption m_options;

    private:
        ::ShaderCompiler m_compiler;
        dal::Filesystem& m_filesys;

    public:
        ShaderSrcManager(dal::Filesystem& filesys)
            : m_filesys(filesys)
            , m_options(filesys)
        {

        }

        std::vector<uint8_t> load(const dal::ResPath& path, const ::ShaderKind shader_kind) {
            const auto cache_path = this->make_shader_cache_path(path, shader_kind);

            if (!this->need_to_compile(cache_path)) {
                auto file = this->m_filesys.open(cache_path);
                return file->read_stl<std::vector<uint8_t>>().value();
            }
            else {
                const auto output = this->load_compile_shader(path, shader_kind);

                auto file = this->m_filesys.open_write(cache_path);
                if (file->is_ready()) {
                    file->write(output.data(), output.size());
                }

                return output;
            }
        }

    private:
        std::vector<uint8_t> load_compile_shader(const dal::ResPath& path, const ::ShaderKind shader_kind) const {
            const auto path_str = path.make_str();

            auto file = this->m_filesys.open(path);
            if (!file->is_ready())
                dalAbort(fmt::format("Failed to open shader file: {}", path_str).c_str());

            const auto data = file->read_stl<std::string>();
            if (!data.has_value())
                dalAbort(fmt::format("Failed to read shader file: {}", path_str).c_str());

            std::vector<uint8_t> output;
            const auto [compile_result, compile_err_msg] = this->m_compiler.compile(
                data->data(),
                data->size(),
                shader_kind,
                path_str.c_str(),
                this->m_options,
                output
            );

            if (!compile_result)
                dalAbort(fmt::format("Failed to compile shader: {}\n{}", path_str, compile_err_msg).c_str());

            dalInfo(fmt::format("Shader compiled: {}", path_str).c_str());
            return output;
        }

        std::string make_shader_cache_path(const dal::ResPath& path, const ::ShaderKind shader_kind) const {
            std::string accum;

            for (auto& x : path.dir_list()) {
                accum += x;
                accum += '-';
            }

            switch (shader_kind) {
                case ::ShaderKind::vert:
                    accum += "vert";
                    break;
                case ::ShaderKind::frag:
                    accum += "frag";
                    break;
                default:
                    dalAbort("Unknown shader kind");
            }

#if DAL_HASH_SHADER_CACHE_NAME
            const auto hashed = std::hash<std::string>{}(accum);
#else
            const auto& hashed = accum;
#endif
            return fmt::format("{}/{}/{}.spv", ::SHADER_CACHE_DIR.u8string(), this->m_options.hash_value(), hashed);
        }

        bool need_to_compile(const std::string& cache_path) {
            if (!this->m_filesys.is_file(cache_path))
                return true;

#if DAL_INVALIDATE_CACHE_ONCE
            if (::g_refreshed_ones.end() == ::g_refreshed_ones.find(cache_path)) {
                ::g_refreshed_ones.insert(cache_path);
                return true;
            }
#endif

            return false;
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

    auto create_vertex_input_state(
        const   VkVertexInputBindingDescription* const binding_descriptions, const uint32_t binding_count,
        const VkVertexInputAttributeDescription* const attrib_descriptions,  const uint32_t attrib_count
    ) {
        VkPipelineVertexInputStateCreateInfo vertex_input_state{};
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertex_input_state.vertexBindingDescriptionCount = binding_count;
        vertex_input_state.pVertexBindingDescriptions = binding_descriptions;
        vertex_input_state.vertexAttributeDescriptionCount = attrib_count;
        vertex_input_state.pVertexAttributeDescriptions = attrib_descriptions;

        return vertex_input_state;
    }

    auto create_info_input_assembly() {
        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        return input_assembly;
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

    auto create_info_viewport_state(
        const VkViewport* const viewports, const uint32_t viewport_count,
        const   VkRect2D* const scissors,  const uint32_t scissor_count
    ) {
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

        viewport_state.viewportCount = viewport_count;
        viewport_state.pViewports = viewports;
        viewport_state.scissorCount = scissor_count;
        viewport_state.pScissors = scissors;

        return viewport_state;
    }

    auto create_info_rasterizer(
        const VkCullModeFlags cull_mode,
        const bool enable_bias,
        const float bias_constant,
        const float bias_slope,
        const bool enable_depth_clamp = false
    ) {
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        rasterizer.depthClampEnable = VK_FALSE;  // vulkan-tutorial.com said this requires GPU feature enabled.

        // Discards all fragents. But why would you ever want it? Well, check the link below.
        // https://stackoverflow.com/questions/42470669/when-does-it-make-sense-to-turn-off-the-rasterization-step
        rasterizer.rasterizerDiscardEnable = VK_FALSE;

        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // Any mode other than FILL requires GPU feature enabled.
        rasterizer.lineWidth = 1;  // GPU feature, `wideLines` required for lines thicker than 1.
        rasterizer.cullMode = cull_mode;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = enable_bias ? VK_TRUE : VK_FALSE;
        rasterizer.depthBiasConstantFactor = bias_constant;
        rasterizer.depthBiasSlopeFactor = bias_slope;
        rasterizer.depthBiasClamp = 0;
        rasterizer.depthClampEnable = enable_depth_clamp ? VK_TRUE : VK_FALSE;

        return rasterizer;
    }

    auto create_info_multisampling() {
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        return multisampling;
    }

    template <size_t _Size, bool _AlphaBlend>
    auto create_info_color_blend_attachment() {
        VkPipelineColorBlendAttachmentState attachment_state_template{};

        attachment_state_template.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachment_state_template.colorBlendOp = VK_BLEND_OP_ADD;
        attachment_state_template.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment_state_template.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachment_state_template.alphaBlendOp = VK_BLEND_OP_ADD;

        if constexpr (_AlphaBlend) {
            attachment_state_template.blendEnable = VK_TRUE;
            attachment_state_template.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            attachment_state_template.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        }
        else {
            attachment_state_template.blendEnable = VK_FALSE;
            attachment_state_template.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment_state_template.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        }

        std::array<VkPipelineColorBlendAttachmentState, _Size> result;

        for (auto& x : result) {
            x = attachment_state_template;
        }

        return result;
    }

    auto create_info_color_blend(
        const VkPipelineColorBlendAttachmentState* const color_blend_attachments,
        const uint32_t attachment_count,
        const bool enable_logic_op
    ) {
        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        color_blending.logicOpEnable = enable_logic_op ? VK_TRUE : VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = attachment_count;
        color_blending.pAttachments = color_blend_attachments;
        color_blending.blendConstants[0] = 0;
        color_blending.blendConstants[1] = 0;
        color_blending.blendConstants[2] = 0;
        color_blending.blendConstants[3] = 0;

        return color_blending;
    }

    auto create_info_depth_stencil(const bool depth_write) {
        VkPipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        depth_stencil.depthTestEnable = VK_TRUE;
        depth_stencil.depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE;
        depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.minDepthBounds = 0;
        depth_stencil.maxDepthBounds = 1;
        depth_stencil.stencilTestEnable = VK_FALSE;
        depth_stencil.front = {};
        depth_stencil.back = {};

        return depth_stencil;
    }

    auto create_info_dynamic_state(const VkDynamicState* const dynamic_states, const uint32_t dynamic_count) {
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

        dynamic_state.dynamicStateCount = dynamic_count;
        dynamic_state.pDynamicStates = dynamic_states;

        return dynamic_state;
    }

    template <typename _Struct>
    auto create_info_push_constant() {
        std::array<VkPushConstantRange, 1> result;

        result[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        result[0].offset = 0;
        result[0].size = sizeof(_Struct);

        return result;
    }

    auto create_pipeline_layout(
        const VkDescriptorSetLayout* const layouts,     const uint32_t layout_count,
        const   VkPushConstantRange* const push_consts, const uint32_t push_const_count,
        const VkDevice logi_device
    ) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        pipelineLayoutInfo.setLayoutCount = layout_count;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = push_const_count;
        pipelineLayoutInfo.pPushConstantRanges = push_consts;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        if ( vkCreatePipelineLayout(logi_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS ) {
            dalAbort("failed to create pipeline layout!");
        }

        return pipelineLayout;
    }

}


namespace {

    dal::ShaderPipeline make_pipeline_gbuf(
        ShaderSrcManager& shader_mgr,
        const bool need_gamma_correction,
        const VkExtent2D& swapchain_extent,
        const VkDescriptorSetLayout desc_layout_simple,
        const VkDescriptorSetLayout desc_layout_per_material,
        const VkDescriptorSetLayout desc_layout_per_actor,
        const VkRenderPass renderpass,
        const uint32_t subpass_index,
        const VkDevice logi_device
    ) {
        const auto vert_src = shader_mgr.load("_asset/glsl/gbuf.vert", ::ShaderKind::vert);
        const auto frag_src = shader_mgr.load("_asset/glsl/gbuf.frag", ::ShaderKind::frag);

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src);
        const ShaderModule frag_shader_module(logi_device, frag_src);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        const auto binding_desc = dal::make_vert_binding_desc_static();
        const auto attrib_desc = dal::make_vert_attrib_desc_static();
        auto vertex_input_state = ::create_vertex_input_state(&binding_desc, 1, attrib_desc.data(), attrib_desc.size());

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(swapchain_extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_BACK_BIT, false, 0, 0);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<3, false>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(true);

        // Dynamic state
        //constexpr std::array<VkDynamicState, 0> dynamic_states{};
        //const auto dynamic_state_info = ::create_info_dynamic_state(dynamic_states.data(), dynamic_states.size());

        // Pipeline layout
        const std::array<VkDescriptorSetLayout, 3> desc_layouts{ desc_layout_simple, desc_layout_per_material, desc_layout_per_actor };
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), nullptr, 0, logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = nullptr;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass;
        pipeline_info.subpass = subpass_index;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            dalAbort("failed to create graphics pipeline!");
        }

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

    dal::ShaderPipeline make_pipeline_gbuf_animated(
        ShaderSrcManager& shader_mgr,
        const bool need_gamma_correction,
        const VkExtent2D& swapchain_extent,
        const VkDescriptorSetLayout desc_layout_simple,
        const VkDescriptorSetLayout desc_layout_per_material,
        const VkDescriptorSetLayout desc_layout_per_actor,
        const VkDescriptorSetLayout desc_layout_animation,
        const VkRenderPass renderpass,
        const uint32_t subpass_index,
        const VkDevice logi_device
    ) {
        const auto vert_src = shader_mgr.load("_asset/glsl/gbuf_animated.vert", ::ShaderKind::vert);
        const auto frag_src = shader_mgr.load("_asset/glsl/gbuf.frag", ::ShaderKind::frag);

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src);
        const ShaderModule frag_shader_module(logi_device, frag_src);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        const auto binding_desc = dal::make_vert_binding_desc_skinned();
        const auto attrib_desc = dal::make_vert_attrib_desc_skinned();
        auto vertex_input_state = ::create_vertex_input_state(&binding_desc, 1, attrib_desc.data(), attrib_desc.size());

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(swapchain_extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_BACK_BIT, false, 0, 0);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<3, false>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(true);

        // Dynamic state
        //constexpr std::array<VkDynamicState, 0> dynamic_states{};
        //const auto dynamic_state_info = ::create_info_dynamic_state(dynamic_states.data(), dynamic_states.size());

        // Pipeline layout
        const std::array<VkDescriptorSetLayout, 4> desc_layouts{ desc_layout_simple, desc_layout_per_material, desc_layout_per_actor, desc_layout_animation };
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), nullptr, 0, logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = nullptr;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass;
        pipeline_info.subpass = subpass_index;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            dalAbort("failed to create graphics pipeline!");
        }

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

    dal::ShaderPipeline make_pipeline_composition(
        ShaderSrcManager& shader_mgr,
        const bool need_gamma_correction,
        const VkExtent2D& extent,
        const VkDescriptorSetLayout desc_layout_composition,
        const VkRenderPass renderpass,
        const uint32_t subpass_index,
        const VkDevice logi_device
    ) {
        const auto vert_src = shader_mgr.load("_asset/glsl/composition.vert", ::ShaderKind::vert);
        const auto frag_src = shader_mgr.load("_asset/glsl/composition.frag", ::ShaderKind::frag);

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src);
        const ShaderModule frag_shader_module(logi_device, frag_src);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        const auto vertex_input_state = ::create_vertex_input_state(nullptr, 0, nullptr, 0);

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_BACK_BIT, false, 0, 0);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<1, false>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(true);

        // Dynamic state
        //constexpr std::array<VkDynamicState, 0> dynamic_states{};
        //const auto dynamic_state_info = ::create_info_dynamic_state(dynamic_states.data(), dynamic_states.size());

        // Pipeline layout
        const std::array<VkDescriptorSetLayout, 1> desc_layouts{ desc_layout_composition };
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), nullptr, 0, logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = nullptr;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass;
        pipeline_info.subpass = subpass_index;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            dalAbort("failed to create graphics pipeline!");
        }

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

    dal::ShaderPipeline make_pipeline_final(
        ShaderSrcManager& shader_mgr,
        const bool need_gamma_correction,
        const VkExtent2D& extent,
        const VkDescriptorSetLayout desc_layout_final,
        const VkRenderPass renderpass,
        const VkDevice logi_device
    ) {
        const auto vert_src = shader_mgr.load("_asset/glsl/fill_screen.vert", ::ShaderKind::vert);
        const auto frag_src = shader_mgr.load("_asset/glsl/fill_screen.frag", ::ShaderKind::frag);

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src);
        const ShaderModule frag_shader_module(logi_device, frag_src);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        auto vertex_input_state = ::create_vertex_input_state(nullptr, 0, nullptr, 0);

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_NONE, false, 0, 0);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<1, false>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(true);

        // Pipeline layout
        const std::array<VkDescriptorSetLayout, 1> desc_layouts{ desc_layout_final };
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), nullptr, 0, logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = nullptr;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            dalAbort("failed to create graphics pipeline!");
        }

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

    dal::ShaderPipeline make_pipeline_alpha(
        ShaderSrcManager& shader_mgr,
        const dal::RenderPass_Alpha& renderpass,
        const bool need_gamma_correction,
        const VkExtent2D& swapchain_extent,
        const VkDescriptorSetLayout desc_layout_alpha,
        const VkDescriptorSetLayout desc_layout_per_material,
        const VkDescriptorSetLayout desc_layout_per_actor,
        const VkDevice logi_device
    ) {
        const auto vert_src = shader_mgr.load("_asset/glsl/alpha.vert", ::ShaderKind::vert);
        const auto frag_src = shader_mgr.load("_asset/glsl/alpha.frag", ::ShaderKind::frag);

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src);
        const ShaderModule frag_shader_module(logi_device, frag_src);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        const auto binding_desc = dal::make_vert_binding_desc_static();
        const auto attrib_desc = dal::make_vert_attrib_desc_static();
        auto vertex_input_state = ::create_vertex_input_state(&binding_desc, 1, attrib_desc.data(), attrib_desc.size());

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(swapchain_extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_BACK_BIT, false, 0, 0);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<1, true>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(false);

        // Dynamic state
        //constexpr std::array<VkDynamicState, 0> dynamic_states{};
        //const auto dynamic_state_info = ::create_info_dynamic_state(dynamic_states.data(), dynamic_states.size());

        // Pipeline layout
        const std::vector<VkDescriptorSetLayout> desc_layouts{
            desc_layout_alpha,
            desc_layout_per_material,
            desc_layout_per_actor,
        };
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), nullptr, 0, logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = nullptr;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass.get();
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            dalAbort("failed to create graphics pipeline!");
        }

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

    dal::ShaderPipeline make_pipeline_alpha_animated(
        ShaderSrcManager& shader_mgr,
        const dal::RenderPass_Alpha& renderpass,
        const bool need_gamma_correction,
        const VkExtent2D& swapchain_extent,
        const VkDescriptorSetLayout desc_layout_alpha,
        const VkDescriptorSetLayout desc_layout_per_material,
        const VkDescriptorSetLayout desc_layout_per_actor,
        const VkDescriptorSetLayout desc_layout_animation,
        const VkDevice logi_device
    ) {
        const auto vert_src = shader_mgr.load("_asset/glsl/alpha_animated.vert", ::ShaderKind::vert);
        const auto frag_src = shader_mgr.load("_asset/glsl/alpha.frag", ::ShaderKind::frag);

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src);
        const ShaderModule frag_shader_module(logi_device, frag_src);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        const auto binding_desc = dal::make_vert_binding_desc_skinned();
        const auto attrib_desc = dal::make_vert_attrib_desc_skinned();
        auto vertex_input_state = ::create_vertex_input_state(&binding_desc, 1, attrib_desc.data(), attrib_desc.size());

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(swapchain_extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_BACK_BIT, false, 0, 0);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<1, true>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(false);

        // Dynamic state
        //constexpr std::array<VkDynamicState, 0> dynamic_states{};
        //const auto dynamic_state_info = ::create_info_dynamic_state(dynamic_states.data(), dynamic_states.size());

        // Pipeline layout
        const std::vector<VkDescriptorSetLayout> desc_layouts{
            desc_layout_alpha,
            desc_layout_per_material,
            desc_layout_per_actor,
            desc_layout_animation,
        };
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), nullptr, 0, logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = nullptr;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass.get();
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            dalAbort("failed to create graphics pipeline!");
        }

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

    dal::ShaderPipeline make_pipeline_shadow(
        ShaderSrcManager& shader_mgr,
        const bool does_support_depth_clamp,
        const VkExtent2D& extent,
        const VkRenderPass renderpass,
        const VkDevice logi_device
    ) {
        const auto vert_src = shader_mgr.load("_asset/glsl/shadow.vert", ::ShaderKind::vert);
        const auto frag_src = shader_mgr.load("_asset/glsl/shadow.frag", ::ShaderKind::frag);

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src);
        const ShaderModule frag_shader_module(logi_device, frag_src);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        const auto binding_desc = dal::make_vert_binding_desc_static();
        const auto attrib_desc = dal::make_vert_attrib_desc_static();
        auto vertex_input_state = ::create_vertex_input_state(&binding_desc, 1, attrib_desc.data(), attrib_desc.size());

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_NONE, true, 80, 8, does_support_depth_clamp);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<3, false>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(true);

        // Dynamic state
        const std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        const auto dynamic_state_info = ::create_info_dynamic_state(dynamic_states.data(), dynamic_states.size());

        // Pipeline layout
        const std::vector<VkDescriptorSetLayout> desc_layouts;
        const auto pc_range = ::create_info_push_constant<dal::U_PC_Shadow>();
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), pc_range.data(), pc_range.size(), logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state_info;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (VK_SUCCESS != vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline))
            dalAbort("failed to create graphics pipeline!");

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

    dal::ShaderPipeline make_pipeline_shadow_animated(
        ShaderSrcManager& shader_mgr,
        const bool does_support_depth_clamp,
        const VkExtent2D& extent,
        const VkRenderPass renderpass,
        const VkDescriptorSetLayout desc_layout_animation,
        const VkDevice logi_device
    ) {
        const auto vert_src = shader_mgr.load("_asset/glsl/shadow_animated.vert", ::ShaderKind::vert);
        const auto frag_src = shader_mgr.load("_asset/glsl/shadow.frag", ::ShaderKind::frag);

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src);
        const ShaderModule frag_shader_module(logi_device, frag_src);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        const auto binding_desc = dal::make_vert_binding_desc_skinned();
        const auto attrib_desc = dal::make_vert_attrib_desc_skinned();
        auto vertex_input_state = ::create_vertex_input_state(&binding_desc, 1, attrib_desc.data(), attrib_desc.size());

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_NONE, true, 80, 8, does_support_depth_clamp);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<3, false>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(true);

        // Dynamic state
        const std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        const auto dynamic_state_info = ::create_info_dynamic_state(dynamic_states.data(), dynamic_states.size());

        // Pipeline layout
        const std::vector<VkDescriptorSetLayout> desc_layouts{ desc_layout_animation };
        const auto pc_range = ::create_info_push_constant<dal::U_PC_Shadow>();
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), pc_range.data(), pc_range.size(), logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state_info;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (VK_SUCCESS != vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline))
            dalAbort("failed to create graphics pipeline!");

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

}


// PipelineManager
namespace dal {

    void PipelineManager::init(
        dal::Filesystem& filesys,
        const bool need_gamma_correction,
        const bool does_support_depth_clamp,
        const VkExtent2D& swapchain_extent,
        const VkExtent2D& gbuf_extent,
        const VkDescriptorSetLayout desc_layout_final,
        const VkDescriptorSetLayout desc_layout_per_global,
        const VkDescriptorSetLayout desc_layout_per_material,
        const VkDescriptorSetLayout desc_layout_per_actor,
        const VkDescriptorSetLayout desc_layout_animation,
        const VkDescriptorSetLayout desc_layout_composition,
        const VkDescriptorSetLayout desc_layout_alpha,
        const RenderPass_Gbuf& rp_gbuf,
        const RenderPass_Final& rp_final,
        const RenderPass_Alpha& rp_alpha,
        const RenderPass_ShadowMap& rp_shadow,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        ::ShaderSrcManager shader_mgr{ filesys };

        shader_mgr.m_options.add_macro_def("DAL_VOLUMETRIC_ATMOS");
        shader_mgr.m_options.add_macro_def("DAL_ATMOS_DITHERING");
        if (need_gamma_correction)
            shader_mgr.m_options.add_macro_def("DAL_GAMMA_CORRECT");

        this->m_gbuf = ::make_pipeline_gbuf(
            shader_mgr,
            need_gamma_correction,
            gbuf_extent,
            desc_layout_per_global,
            desc_layout_per_material,
            desc_layout_per_actor,
            rp_gbuf.get(), 0,
            logi_device
        );

        this->m_gbuf_animated = ::make_pipeline_gbuf_animated(
            shader_mgr,
            need_gamma_correction,
            gbuf_extent,
            desc_layout_per_global,
            desc_layout_per_material,
            desc_layout_per_actor,
            desc_layout_animation,
            rp_gbuf.get(), 0,
            logi_device
        );

        this->m_composition = ::make_pipeline_composition(
            shader_mgr,
            need_gamma_correction,
            gbuf_extent,
            desc_layout_composition,
            rp_gbuf.get(), 1,
            logi_device
        );

        this->m_final = ::make_pipeline_final(
            shader_mgr,
            need_gamma_correction,
            swapchain_extent,
            desc_layout_final,
            rp_final.get(),
            logi_device
        );

        this->m_alpha = ::make_pipeline_alpha(
            shader_mgr,
            rp_alpha,
            need_gamma_correction,
            gbuf_extent,
            desc_layout_alpha,
            desc_layout_per_material,
            desc_layout_per_actor,
            logi_device
        );

        this->m_alpha_animated = ::make_pipeline_alpha_animated(
            shader_mgr,
            rp_alpha,
            need_gamma_correction,
            gbuf_extent,
            desc_layout_alpha,
            desc_layout_per_material,
            desc_layout_per_actor,
            desc_layout_animation,
            logi_device
        );

        this->m_shadow = ::make_pipeline_shadow(
            shader_mgr,
            does_support_depth_clamp,
            VkExtent2D{ 512, 512 },
            rp_shadow.get(),
            logi_device
        );

        this->m_shadow_animated = ::make_pipeline_shadow_animated(
            shader_mgr,
            does_support_depth_clamp,
            VkExtent2D{ 512, 512 },
            rp_shadow.get(),
            desc_layout_animation,
            logi_device
        );
    }

    void PipelineManager::destroy(const VkDevice logi_device) {
        this->m_gbuf.destroy(logi_device);
        this->m_gbuf_animated.destroy(logi_device);
        this->m_composition.destroy(logi_device);
        this->m_final.destroy(logi_device);
        this->m_alpha.destroy(logi_device);
        this->m_alpha_animated.destroy(logi_device);
        this->m_shadow.destroy(logi_device);
        this->m_shadow_animated.destroy(logi_device);
    }

}
