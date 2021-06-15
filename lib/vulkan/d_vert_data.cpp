#include "d_vert_data.h"

#include "d_logger.h"


namespace dal {

    VkVertexInputBindingDescription make_vert_binding_desc_static() {
        VkVertexInputBindingDescription result{};

        result.binding = 0;
        result.stride = sizeof(dal::VertexStatic);
        result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return result;
    }

    std::array<VkVertexInputAttributeDescription, 3> make_vert_attrib_desc_static() {
        std::array<VkVertexInputAttributeDescription, 3> result{};

        result[0].binding = 0;
        result[0].location = 0;
        result[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[0].offset = offsetof(dal::VertexStatic, m_pos);

        result[1].binding = 0;
        result[1].location = 1;
        result[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[1].offset = offsetof(dal::VertexStatic, m_normal);

        result[2].binding = 0;
        result[2].location = 2;
        result[2].format = VK_FORMAT_R32G32_SFLOAT;
        result[2].offset = offsetof(dal::VertexStatic, m_uv_coord);

        return result;
    }

    VkVertexInputBindingDescription make_vert_binding_desc_skinned() {
        VkVertexInputBindingDescription result{};

        result.binding = 0;
        result.stride = sizeof(dal::VertexSkinned);
        result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return result;
    }

    std::array<VkVertexInputAttributeDescription, 5> make_vert_attrib_desc_skinned() {
        std::array<VkVertexInputAttributeDescription, 5> result{};

        result[0].binding = 0;
        result[0].location = 0;
        result[0].format = VK_FORMAT_R32G32B32A32_SINT;
        result[0].offset = offsetof(dal::VertexSkinned, m_joint_ids);

        result[1].binding = 0;
        result[1].location = 1;
        result[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        result[1].offset = offsetof(dal::VertexSkinned, m_joint_weights);

        result[2].binding = 0;
        result[2].location = 2;
        result[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[2].offset = offsetof(dal::VertexSkinned, m_pos);

        result[3].binding = 0;
        result[3].location = 3;
        result[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[3].offset = offsetof(dal::VertexSkinned, m_normal);

        result[4].binding = 0;
        result[4].location = 4;
        result[4].format = VK_FORMAT_R32G32_SFLOAT;
        result[4].offset = offsetof(dal::VertexSkinned, m_uv_coord);

        return result;
    }

}


// VertexBuffer
namespace dal {

    void VertexBuffer::init(
        const std::vector<VertexStatic>& vertices,
        const std::vector<index_data_t>& indices,
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_index_size = indices.size();

        // Build vertex buffer
        // -----------------------------------------------------------------------------
        {
            const VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

            BufferMemory staging_buffer;
            const auto result_staging = staging_buffer.init(
                buffer_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                phys_device,
                logi_device
            );
            dalAssert(result_staging);
            staging_buffer.copy_from_mem(vertices.data(), buffer_size, logi_device);

            const auto result_this = this->m_vertices.init(
                buffer_size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                phys_device,
                logi_device
            );
            dalAssert(result_this);

            this->m_vertices.copy_from_buf(staging_buffer, buffer_size, cmd_pool, graphics_queue, logi_device);
            staging_buffer.destroy(logi_device);
        }

        // Build index buffer
        // -----------------------------------------------------------------------------
        {
            const VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

            BufferMemory staging_buffer;
            const auto result_staging = staging_buffer.init(
                buffer_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                phys_device,
                logi_device
            );
            dalAssert(result_staging);
            staging_buffer.copy_from_mem(indices.data(), buffer_size, logi_device);

            const auto result_this = this->m_indices.init(
                buffer_size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                phys_device,
                logi_device
            );
            dalAssert(result_this);

            this->m_indices.copy_from_buf(staging_buffer, buffer_size, cmd_pool, graphics_queue, logi_device);
            staging_buffer.destroy(logi_device);
        }
    }

    void VertexBuffer::destroy(const VkDevice logi_device) {
        this->m_vertices.destroy(logi_device);
        this->m_indices.destroy(logi_device);
        this->m_index_size = 0;
    }

}
