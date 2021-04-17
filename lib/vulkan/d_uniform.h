#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "d_vulkan_header.h"
#include "d_buffer_memory.h"


namespace dal {

    struct U_PerFrame {
        glm::mat4 m_view{1}, m_proj{1};
    };

    struct U_PerMaterial {
        float m_roughness = 0.5;
        float m_metallic = 0;
    };

    struct U_PerActor {
        glm::mat4 m_model{1};
    };


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


    class DescSetLayoutManager {

    private:
        VkDescriptorSetLayout m_layout_final = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_layout_simple = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_layout_per_material = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_layout_per_actor = VK_NULL_HANDLE;

    public:
        void init(const VkDevice logiDevice);

        void destroy(const VkDevice logiDevice);

        auto& layout_final() const {
            return this->m_layout_final;
        }

        auto& layout_simple() const {
            return this->m_layout_simple;
        }

        auto layout_per_material() const {
            return this->m_layout_per_material;
        }

        auto layout_per_actor() const {
            return this->m_layout_per_actor;
        }

    };


    class DescSet {

    private:
        VkDescriptorSet m_handle = VK_NULL_HANDLE;

    public:
        void set(const VkDescriptorSet desc_set) {
            this->m_handle = desc_set;
        }

        bool is_ready() const;

        auto& get() const {
            return this->m_handle;
        }

        void record_final(
            const VkImageView color_view,
            const VkSampler sampler,
            const VkDevice logi_device
        );

        void record_simple(
            const UniformBuffer<U_PerFrame>& ubuf_per_frame,
            const VkDevice logi_device
        );

        void record_material(
            const UniformBuffer<U_PerMaterial>& ubuf_per_material,
            const VkImageView texture_view,
            const VkSampler sampler,
            const VkDevice logi_device
        );

        void record_per_actor(
            const UniformBuffer<U_PerActor>& ubuf_per_actor,
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

        DescSet allocate(const VkDescriptorSetLayout layout, const VkDevice logi_device);

        std::vector<DescSet> allocate(const uint32_t count, const VkDescriptorSetLayout layout, const VkDevice logi_device);

        auto get() {
            return this->m_handle;
        }
        auto get() const {
            return this->m_handle;
        }

    };


    class DescriptorManager {

    private:
        DescPool m_pool, m_pool_final;
        std::vector<DescSet> m_descset_simple;
        std::vector<DescSet> m_descset_final;

    public:
        void init(const uint32_t swapchain_count, const VkDevice logi_device);

        void destroy(VkDevice logiDevice);

        auto& pool() {
            return this->m_pool;
        }

        void init_desc_sets_simple(
            const dal::UniformBufferArray<U_PerFrame>& ubufs_simple,
            const uint32_t swapchain_count,
            const VkDescriptorSetLayout desc_layout_simple,
            const VkDevice logi_device
        );

        void init_desc_sets_final(
            const std::vector<VkImageView>& color_views,
            const uint32_t swapchain_count,
            const VkSampler sampler,
            const VkDescriptorSetLayout desc_layout_final,
            const VkDevice logi_device
        );

        auto& desc_set_simple() {
            return this->m_descset_simple;
        }

        std::vector<VkDescriptorSet> desc_set_raw_simple() const;

        auto& desc_set_final_at(const size_t index) const {
            return this->m_descset_final.at(index).get();
        }

    };

}
