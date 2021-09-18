#pragma once

#include <deque>
#include <vector>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "d_konsts.h"
#include "d_image_obj.h"
#include "d_vulkan_header.h"
#include "d_buffer_memory.h"


// Uniform buffer structs
namespace dal {

    struct U_PC_Shadow {
        glm::mat4 m_model_mat;
        glm::mat4 m_light_mat;
    };

    struct U_PC_Simple {
        glm::mat4 m_model_mat;
        glm::mat4 m_proj_view_mat;
    };

    struct U_PC_Mirror {
        glm::mat4 m_proj_view_mat;
        glm::mat4 m_model_mat;
    };


    struct U_Shader_Final {
        glm::mat4 m_rotation;
    };

    struct U_CameraTransform {
        glm::mat4 m_view;
        glm::mat4 m_proj;
        glm::mat4 m_view_inv;
        glm::mat4 m_proj_inv;

        glm::vec4 m_view_pos;

        float m_near, m_far;
    };

    struct U_PerMaterial {
        float m_roughness = 0.5;
        float m_metallic = 0;
    };

    struct U_PerActor {
        glm::mat4 m_model{1};
    };

    struct U_AnimTransform {
        glm::mat4 m_transforms[dal::MAX_JOINT_COUNT];
    };

    struct U_GlobalLight {
        glm::mat4 m_dlight_mat[dal::MAX_DLIGHT_COUNT];
        glm::vec4 m_dlight_direc[dal::MAX_DLIGHT_COUNT];
        glm::vec4 m_dlight_color[dal::MAX_DLIGHT_COUNT];

        glm::vec4 m_plight_pos_n_max_dist[dal::MAX_PLIGHT_COUNT];
        glm::vec4 m_plight_color[dal::MAX_PLIGHT_COUNT];

        glm::mat4 m_slight_mat[dal::MAX_SLIGHT_COUNT];
        glm::vec4 m_slight_pos_n_max_dist[dal::MAX_SLIGHT_COUNT];
        glm::vec4 m_slight_direc_n_fade_start[dal::MAX_SLIGHT_COUNT];
        glm::vec4 m_slight_color_n_fade_end[dal::MAX_SLIGHT_COUNT];

        glm::vec4 m_ambient_light;

        glm::vec4 m_dlight_clip_dist;

        float m_atmos_intensity;
        float m_mie_scattering_coeff;

        uint32_t m_dlight_count = 0;
        uint32_t m_plight_count = 0;
        uint32_t m_slight_count = 0;
    };

}


// Uniform buffer object
namespace dal {

    template <typename _DataStruct>
    class UniformBuffer {

    private:
        BufferMemory m_buffer;

    public:
        bool init(const VkPhysicalDevice phys_device, const VkDevice logi_device) {
            this->destroy(logi_device);

            return this->m_buffer.init(
                this->data_size(),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                phys_device,
                logi_device
            );
        }

        void destroy(const VkDevice logi_device) {
            this->m_buffer.destroy(logi_device);
        }

        bool is_ready() const {
            return this->m_buffer.is_ready();
        }

        constexpr uint32_t data_size() const {
            return sizeof(_DataStruct);
        }

        VkBuffer buffer() const {
            return this->m_buffer.buffer();
        }

        void copy_to_buffer(const _DataStruct& data, const VkDevice logi_device) {
            this->m_buffer.copy_from_mem(&data, this->data_size(), logi_device);
        }

    };


    template <typename _DataStruct>
    class UniformBufferArray {

    private:
        std::vector<UniformBuffer<_DataStruct>> m_buffers;

    public:
        void init(const uint32_t array_size, const VkPhysicalDevice phys_device, const VkDevice logi_device) {
            this->destroy(logi_device);

            for (uint32_t i = 0; i < array_size; ++i) {
                this->m_buffers.emplace_back().init(phys_device, logi_device);
            }
        }

        void destroy(const VkDevice logi_device) {
            for (auto& x : this->m_buffers) {
                x.destroy(logi_device);
            }
            this->m_buffers.clear();
        }

        constexpr uint32_t data_size() const {
            return sizeof(_DataStruct);
        }

        uint32_t array_size() const {
            return this->m_buffers.size();
        }

        auto& at(const uint32_t index) {
            return this->m_buffers.at(index);
        }
        auto& at(const uint32_t index) const {
            return this->m_buffers.at(index);
        }

        void copy_to_buffer(const size_t index, const _DataStruct& data, const VkDevice logi_device) {
            this->m_buffers.at(index).copy_to_buffer(data, logi_device);
        }

    };

}


// Descriptor set layouts
namespace dal {

    class IDescSetLayout {

    private:
        VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;

    public:
        IDescSetLayout() = default;

        IDescSetLayout(const IDescSetLayout&) = delete;
        IDescSetLayout& operator=(const IDescSetLayout&) = delete;

    protected:
        void reset(const VkDescriptorSetLayout v, const VkDevice logi_device) noexcept {
            this->destroy(logi_device);
            this->m_layout = v;
        }

        void build(const VkDescriptorSetLayoutCreateInfo& create_info, const VkDevice logi_device) noexcept;

    public:
        void destroy(const VkDevice logi_device) noexcept;

        VkDescriptorSetLayout get() const noexcept {
            return this->m_layout;
        }

        bool is_ready() const noexcept {
            return VK_NULL_HANDLE != this->m_layout;
        }

    };


    struct DescLayout_Final : public IDescSetLayout {
        void init(const VkDevice logi_device);
    };

    struct DescLayout_PerGlobal : public IDescSetLayout {
        void init(const VkDevice logi_device);
    };

    struct DescLayout_PerMaterial : public IDescSetLayout {
        void init(const VkDevice logi_device);
    };

    struct DescLayout_PerActor : public IDescSetLayout {
        void init(const VkDevice logi_device);
    };

    struct DescLayout_ActorAnimated : public IDescSetLayout {
        void init(const VkDevice logi_device);
    };

    struct DescLayout_Composition : public IDescSetLayout {
        void init(const VkDevice logi_device);
    };

    struct DescLayout_Alpha : public IDescSetLayout {
        void init(const VkDevice logi_device);
    };

    struct DescLayout_Mirror : public IDescSetLayout {
        void init(const VkDevice logi_device);
    };


    class DescSetLayoutManager {

    private:
        DescLayout_Final m_layout_final;

        DescLayout_PerGlobal m_layout_per_global;
        DescLayout_PerMaterial m_layout_per_material;
        DescLayout_PerActor m_layout_per_actor;
        DescLayout_ActorAnimated m_layout_actor_animated;

        DescLayout_Composition m_layout_composition;
        DescLayout_Alpha m_layout_alpha;
        DescLayout_Mirror m_layout_mirror;

    public:
        void init(const VkDevice logiDevice);

        void destroy(const VkDevice logiDevice);

        auto& layout_final() const {
            return this->m_layout_final;
        }

        auto& layout_per_global() const {
            return this->m_layout_per_global;
        }

        auto& layout_per_material() const {
            return this->m_layout_per_material;
        }

        auto& layout_per_actor() const {
            return this->m_layout_per_actor;
        }

        auto& layout_actor_animated() const {
            return this->m_layout_actor_animated;
        }

        auto& layout_composition() const {
            return this->m_layout_composition;
        }

        auto& layout_alpha() const {
            return this->m_layout_alpha;
        }

        auto& layout_mirror() const {
            return this->m_layout_mirror;
        }

    };

}


namespace dal {

    class DescSet {

    private:
        VkDescriptorSet m_handle = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_layout;

    public:
        DescSet(const DescSet&) = delete;
        DescSet& operator=(const DescSet&) = delete;

    public:
        DescSet() = default;

        DescSet(DescSet&&);

        DescSet& operator=(DescSet&&);

        void set(const VkDescriptorSet desc_set, const VkDescriptorSetLayout layout) {
            this->m_handle = desc_set;
            this->m_layout = layout;
        }

        bool is_ready() const;

        auto& get() const {
            return this->m_handle;
        }

        auto& layout() const {
            return this->m_layout;
        }

        void record_final(
            const VkImageView color_view,
            const SamplerTexture& sampler,
            const UniformBuffer<U_Shader_Final>& ubuf_per_frame,
            const VkDevice logi_device
        );

        void record_per_global(
            const UniformBuffer<U_CameraTransform>& ubuf_per_frame,
            const UniformBuffer<U_GlobalLight>& ubuf_global_light,
            const VkDevice logi_device
        );

        void record_material(
            const UniformBuffer<U_PerMaterial>& ubuf_per_material,
            const VkImageView texture_view,
            const SamplerTexture& sampler,
            const VkDevice logi_device
        );

        void record_per_actor(
            const UniformBuffer<U_PerActor>& ubuf_per_actor,
            const VkDevice logi_device
        );

        void record_actor_animated(
            const UniformBuffer<U_PerActor>& ubuf_per_actor,
            const UniformBuffer<U_AnimTransform>& ubuf_animation,
            const VkDevice logi_device
        );

        void record_composition(
            const std::vector<VkImageView>& attachment_views,
            const UniformBuffer<U_GlobalLight>& ubuf_global_light,
            const UniformBuffer<U_CameraTransform>& ubuf_per_frame,
            const std::array<VkImageView, dal::MAX_DLIGHT_COUNT>& dlight_shadow_maps,
            const std::array<VkImageView, dal::MAX_SLIGHT_COUNT>& slight_shadow_maps,
            const SamplerDepth& sampler,
            const VkDevice logi_device
        );

        void record_alpha(
            const UniformBuffer<U_CameraTransform>& ubuf_per_frame,
            const UniformBuffer<U_GlobalLight>& ubuf_global_light,
            const std::array<VkImageView, dal::MAX_DLIGHT_COUNT>& dlight_shadow_maps,
            const std::array<VkImageView, dal::MAX_SLIGHT_COUNT>& slight_shadow_maps,
            const SamplerDepth& sampler,
            const VkDevice logi_device
        );

    };


    class DescPool {

    private:
        VkDescriptorPool m_handle = VK_NULL_HANDLE;

    public:
        void init(
            const uint32_t uniform_buf_count,
            const uint32_t image_sampler_count,
            const uint32_t input_attachment_count,
            const uint32_t desc_set_count,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        void reset(const VkDevice logi_device);

        DescSet allocate(const dal::IDescSetLayout& layout, const VkDevice logi_device);

        std::vector<DescSet> allocate(const uint32_t count, const dal::IDescSetLayout& layout, const VkDevice logi_device);

        auto get() {
            return this->m_handle;
        }
        auto get() const {
            return this->m_handle;
        }

    };


    class DescAllocator {

    private:
        DescPool m_pool;
        std::unordered_map<VkDescriptorSetLayout, std::deque<DescSet>> m_waiting_queue;
        size_t m_allocated_outside = 0;

    public:
        void init(
            const uint32_t uniform_buf_count,
            const uint32_t image_sampler_count,
            const uint32_t input_attachment_count,
            const uint32_t desc_set_count,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        void reset(const VkDevice logi_device);

        DescSet allocate(const dal::IDescSetLayout& layout, const VkDevice logi_device);

        std::vector<DescSet> allocate(const uint32_t count, const dal::IDescSetLayout& layout, const VkDevice logi_device);

        void free(DescSet&& desc_set);

    private:
        std::deque<DescSet>& get_queue(const VkDescriptorSetLayout layout);

    };


    class DescriptorManager {

    private:
        DescPool m_pool;
        std::vector<DescSet> m_descset_per_global;
        std::vector<DescSet> m_descset_final;
        std::vector<DescSet> m_descset_composition;
        std::vector<DescSet> m_descset_alpha;

    public:
        void init(const uint32_t swapchain_count, const VkDevice logi_device);

        void destroy(VkDevice logiDevice);

        auto& pool() {
            return this->m_pool;
        }

        DescSet& add_descset_per_global(const dal::DescLayout_PerGlobal& desc_layout_per_global, const VkDevice logi_device);

        DescSet& add_descset_final(const dal::DescLayout_Final& desc_layout_final, const VkDevice logi_device);

        DescSet& add_descset_composition(const dal::DescLayout_Composition& desc_layout_composition, const VkDevice logi_device);

        DescSet& add_descset_alpha(const dal::DescLayout_Alpha& desc_layout_alpha, const VkDevice logi_device);

        auto& desc_set_per_global_at(const size_t index) const {
            return this->m_descset_per_global.at(index).get();
        }

        auto& desc_set_final_at(const size_t index) const {
            return this->m_descset_final.at(index).get();
        }

        auto& desc_set_composition_at(const size_t index) const {
            return this->m_descset_composition.at(index);
        }

        auto& desc_set_alpha_at(const size_t index) const {
            return this->m_descset_alpha.at(index).get();
        }

    };

}
