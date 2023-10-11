#include "d_command.h"

#include "dal/util/logger.h"


// CommandPool
namespace dal {

    void CommandPool::init(const uint32_t queue_family_index, const VkDevice logi_device) {
        this->destroy(logi_device);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queue_family_index;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(logi_device, &poolInfo, nullptr, &this->m_handle) != VK_SUCCESS) {
            dalAbort("failed to create command pool!");
        }
    }

    void CommandPool::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_handle) {
            vkDestroyCommandPool(logi_device, this->m_handle, nullptr);
            this->m_handle = VK_NULL_HANDLE;
        }
    }

    void CommandPool::allocate(VkCommandBuffer* const alloc_dst, const uint32_t alloc_count, const VkDevice logi_device) const {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = this->m_handle;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = alloc_count;

        if (VK_SUCCESS != vkAllocateCommandBuffers(logi_device, &allocInfo, alloc_dst)) {
            dalAbort("failed to allocate command buffers!");
        }
    }

    std::vector<VkCommandBuffer> CommandPool::allocate(const uint32_t alloc_count, const VkDevice logi_device) const {
        std::vector<VkCommandBuffer> cmd_buffers(alloc_count);
        this->allocate(cmd_buffers.data(), alloc_count, logi_device);
        return cmd_buffers;
    }

    void CommandPool::free(const VkCommandBuffer cmd_buf, const VkDevice logi_device) const {
        vkFreeCommandBuffers(logi_device, this->m_handle, 1, &cmd_buf);
    }

    void CommandPool::free(const std::vector<VkCommandBuffer>& cmd_buf, const VkDevice logi_device) const {
        vkFreeCommandBuffers(logi_device, this->m_handle, cmd_buf.size(), cmd_buf.data());
    }

    VkCommandBuffer CommandPool::begin_single_time_cmd(const VkDevice logi_device) {
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = this->m_handle;
        alloc_info.commandBufferCount = 1;
        vkAllocateCommandBuffers(logi_device, &alloc_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        return command_buffer;
    }

    void CommandPool::end_single_time_cmd(
        const VkCommandBuffer cmd_buf,
        const VkQueue graphics_queue,
        const VkDevice logi_device
    ) {
        vkEndCommandBuffer(cmd_buf);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd_buf;

        vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphics_queue);

        vkFreeCommandBuffers(logi_device, this->m_handle, 1, &cmd_buf);
    }

}
