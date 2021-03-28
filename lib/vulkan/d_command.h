#pragma once

#include <vector>

#include "d_vulkan_header.h"


namespace dal {

    class CommandPool {

    private:
        VkCommandPool m_handle = VK_NULL_HANDLE;

    public:
        CommandPool() = default;
        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;

    public:
        CommandPool(CommandPool&& other) {
            std::swap(this->m_handle, other.m_handle);
        }

        CommandPool& operator=(CommandPool&& other) {
            std::swap(this->m_handle, other.m_handle);
            return *this;
        }

        void init(const uint32_t queue_family_index, const VkDevice logi_device);

        void destroy(const VkDevice logi_device);

        void allocate(VkCommandBuffer* const alloc_dst, const uint32_t alloc_count, const VkDevice logi_device) const;

        std::vector<VkCommandBuffer> allocate(const uint32_t alloc_count, const VkDevice logi_device) const;

        VkCommandBuffer begin_single_time_cmd(const VkDevice logi_device);

        void end_single_time_cmd(
            const VkCommandBuffer cmd_buf,
            const VkQueue graphics_queue,
            const VkDevice logi_device
        );

    };


    class CmdPoolManager {

    private:
        std::vector<CommandPool> m_pools;  // Per swapchain
        std::vector<VkCommandBuffer> m_cmd_buffers;  // Per swapchain
        CommandPool m_pool_for_single_time;

    public:
        void init(const uint32_t swapchain_count, const uint32_t queue_family_index, const VkDevice logi_device);

        void destroy(const VkDevice logi_device);

        void record_all_simple(
            const std::vector<VkFramebuffer>& swapchain_fbufs,
            const VkExtent2D& swapchain_extent,
            const VkBuffer vertex_buffer,
            const uint32_t vertex_size,
            const VkRenderPass render_pass,
            const VkPipeline graphics_pipeline
        );

        auto& cmd_buffer_at(const size_t index) const {
            return this->m_cmd_buffers.at(index);
        }

        auto& pool_single_time() {
            return this->m_pool_for_single_time;
        }

    };

}
