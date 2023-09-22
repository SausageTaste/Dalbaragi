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

        void free(const VkCommandBuffer cmd_buf, const VkDevice logi_device) const;

        void free(const std::vector<VkCommandBuffer>& cmd_buf, const VkDevice logi_device) const;

        VkCommandBuffer begin_single_time_cmd(const VkDevice logi_device);

        void end_single_time_cmd(
            const VkCommandBuffer cmd_buf,
            const VkQueue graphics_queue,
            const VkDevice logi_device
        );

    };

}
