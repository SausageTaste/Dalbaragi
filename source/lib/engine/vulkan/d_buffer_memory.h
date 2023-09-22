#pragma once

#include "d_vulkan_header.h"
#include "d_command.h"


namespace dal {

    uint32_t find_memory_type(
        const uint32_t typeFilter,
        const VkMemoryPropertyFlags props,
        const VkPhysicalDevice phys_device
    );


    class BufferMemory {

    private:
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkDeviceSize m_size = 0;

    public:
        BufferMemory() = default;

        BufferMemory(const BufferMemory&) = delete;
        BufferMemory& operator=(const BufferMemory&) = delete;

    public:
        ~BufferMemory();

        BufferMemory(BufferMemory&& other) noexcept;

        BufferMemory& operator=(BufferMemory&& other) noexcept;

        [[nodiscard]]
        bool init(
            const VkDeviceSize size,
            const VkBufferUsageFlags usage,
            const VkMemoryPropertyFlags properties,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        bool is_ready() const;

        auto size() const {
            return this->m_size;
        }

        auto buffer()  {
            return this->m_buffer;
        }
        auto buffer() const {
            return this->m_buffer;
        }

        void copy_from_mem(
            const void* src,
            const size_t size,
            const VkDevice logi_device
        );

        void copy_from_buf(
            const BufferMemory& other,
            const VkDeviceSize size,
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkDevice logi_device
        );

    };

}
