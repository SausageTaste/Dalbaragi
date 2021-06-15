#pragma once

#include <array>

#include "d_model_data.h"
#include "d_vulkan_header.h"
#include "d_command.h"
#include "d_buffer_memory.h"


namespace dal {

    VkVertexInputBindingDescription make_vert_binding_desc_static();
    std::array<VkVertexInputAttributeDescription, 3> make_vert_attrib_desc_static();

    VkVertexInputBindingDescription make_vert_binding_desc_skinned();
    std::array<VkVertexInputAttributeDescription, 5> make_vert_attrib_desc_skinned();


    using index_data_t = uint32_t;


    class VertexBuffer {

    private:
        BufferMemory m_vertices, m_indices;
        uint32_t m_index_size = 0;

    public:
        void init(
            const std::vector<VertexStatic>& vertices,
            const std::vector<index_data_t>& indices,
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto index_size() const {
            return this->m_index_size;
        }

        auto vertex_buffer()  {
            return this->m_vertices.buffer();
        }
        auto vertex_buffer() const {
            return this->m_vertices.buffer();
        }

        auto index_buffer()  {
            return this->m_indices.buffer();
        }
        auto index_buffer() const {
            return this->m_indices.buffer();
        }

    };

}
