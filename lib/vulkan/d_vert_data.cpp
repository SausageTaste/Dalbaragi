#include "d_vert_data.h"

#include <fmt/format.h>

#include "d_logger.h"


namespace {

    uint32_t find_memory_type(
        const uint32_t typeFilter,
        const VkMemoryPropertyFlags props,
        const VkPhysicalDevice phys_device
    ) {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(phys_device, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
                return i;
            }
        }

        dalAbort("failed to find suitable memory type!");
    }

    std::pair<VkBuffer, VkDeviceMemory> create_buffer(
        const VkDeviceSize size,
        const VkBufferUsageFlags usage,
        const VkMemoryPropertyFlags properties,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        std::pair<VkBuffer, VkDeviceMemory> output{ VK_NULL_HANDLE, VK_NULL_HANDLE };

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logi_device, &bufferInfo, nullptr, &output.first) != VK_SUCCESS) {
            dalError("failed to create buffer!");
            return { VK_NULL_HANDLE, VK_NULL_HANDLE };
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logi_device, output.first, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = ::find_memory_type(
            memRequirements.memoryTypeBits, properties, phys_device
        );

        if (vkAllocateMemory(logi_device, &allocInfo, nullptr, &output.second) != VK_SUCCESS) {
            dalError("failed to allocate buffer memory!");
            return { VK_NULL_HANDLE, VK_NULL_HANDLE };
        }

        vkBindBufferMemory(logi_device, output.first, output.second, 0);
        return output;
    }

    void copy_buffer(
        VkBuffer dst_buffer,
        VkBuffer src_buffer,
        dal::CommandPool& cmd_pool,
        VkDeviceSize size,
        VkQueue graphics_queue,
        VkDevice logi_device
    ) {
        const auto commandBuffer = cmd_pool.begin_single_time_cmd(logi_device);

        VkBufferCopy copy_region{};
        copy_region.size = size;
        vkCmdCopyBuffer(commandBuffer, src_buffer, dst_buffer, 1, &copy_region);

        cmd_pool.end_single_time_cmd(commandBuffer, graphics_queue, logi_device);
    }

}


namespace dal {

    VkVertexInputBindingDescription make_vert_binding_desc() {
        VkVertexInputBindingDescription result{};

        result.binding = 0;
        result.stride = sizeof(dal::Vertex);
        result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return result;
    }

    std::array<VkVertexInputAttributeDescription, 3> make_vert_attribute_descriptions() {
        std::array<VkVertexInputAttributeDescription, 3> result{};

        result[0].binding = 0;
        result[0].location = 0;
        result[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[0].offset = offsetof(Vertex, m_pos);

        result[1].binding = 0;
        result[1].location = 1;
        result[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[1].offset = offsetof(Vertex, m_normal);

        result[2].binding = 0;
        result[2].location = 2;
        result[2].format = VK_FORMAT_R32G32_SFLOAT;
        result[2].offset = offsetof(Vertex, m_uv_coord);

        return result;
    }

}


// BufferMemory
namespace dal {

    BufferMemory::~BufferMemory() {
        dalAssert(!this->is_ready());
    }

    bool BufferMemory::init(
        const VkDeviceSize size,
        const VkBufferUsageFlags usage,
        const VkMemoryPropertyFlags properties,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_size = size;
        std::tie(this->m_buffer, this->m_memory) = ::create_buffer(
            size, usage, properties, phys_device, logi_device
        );

        return this->is_ready();
    }

    void BufferMemory::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_buffer) {
            vkDestroyBuffer(logi_device, this->m_buffer, nullptr);
            this->m_buffer = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_memory) {
            vkFreeMemory(logi_device, this->m_memory, nullptr);
            this->m_memory = VK_NULL_HANDLE;
        }

        this->m_size = 0;
    }

    bool BufferMemory::is_ready() const {
        return this->m_buffer != VK_NULL_HANDLE && this->m_memory != VK_NULL_HANDLE;
    }

    void BufferMemory::copy_mem_from(
        const void* src,
        const size_t size,
        const VkDevice logi_device
    ) {
        dalAssert(this->is_ready());

        dalAssertm(
            this->size() >= size,
            fmt::format(
                "Requested copy size if bigger than buffer size: req: {}, this: {}",
                size, this->size()
            ).c_str()
        );

        void* ptr = nullptr;
        vkMapMemory(logi_device, this->m_memory, 0, this->size(), 0, &ptr);
        memcpy(ptr, src, size);
        vkUnmapMemory(logi_device, this->m_memory);
    }

    void BufferMemory::copy_buf_from(
        const BufferMemory& src,
        const VkDeviceSize size,
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkDevice logi_device
    ) {
        dalAssert(this->is_ready());

        dalAssertm(
            std::min<VkDeviceSize>(this->size(), src.size()) >= size,
            fmt::format(
                "Requested copy size if bigger than buffer size: req: {}, this: {}, other {}",
                size, this->size(), src.size()
            ).c_str()
        );

        ::copy_buffer(this->m_buffer, src.m_buffer, cmd_pool, size, graphics_queue, logi_device);
    }

}


// VertexBuffer
namespace dal {

    void VertexBuffer::init(
        const std::vector<Vertex>& vertices,
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_vert_size = vertices.size();
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
        staging_buffer.copy_mem_from(vertices.data(), buffer_size, logi_device);

        const auto result_this = this->m_buffer.init(
            buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            phys_device,
            logi_device
        );
        dalAssert(result_this);

        this->m_buffer.copy_buf_from(staging_buffer, buffer_size, cmd_pool, graphics_queue, logi_device);
        staging_buffer.destroy(logi_device);
    }

    void VertexBuffer::destroy(const VkDevice logi_device) {
        this->m_buffer.destroy(logi_device);
        this->m_vert_size = 0;
    }

}
