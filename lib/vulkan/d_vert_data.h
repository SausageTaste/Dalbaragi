#pragma once

#include <array>

#include "d_model_data.h"
#include "d_vulkan_header.h"
#include "d_command.h"


namespace dal {

    VkVertexInputBindingDescription make_vert_binding_desc();
    std::array<VkVertexInputAttributeDescription, 3> make_vert_attribute_descriptions();


    class BufferMemory {

    private:
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkDeviceSize m_size = 0;

    public:
        ~BufferMemory();

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

        void copy_mem_from(
            const void* src,
            const size_t size,
            const VkDevice logi_device
        );

        void copy_buf_from(
            const BufferMemory& other,
            const VkDeviceSize size,
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkDevice logi_device
        );

    };


    class VertexBuffer {

    private:
        BufferMemory m_buffer;
        uint32_t m_vert_size = 0;

    public:
        void init(
            const std::vector<Vertex>& vertices,
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto vert_size() const {
            return this->m_vert_size;
        }

        auto buffer()  {
            return this->m_buffer.buffer();
        }
        auto buffer() const {
            return this->m_buffer.buffer();
        }

    };

}
