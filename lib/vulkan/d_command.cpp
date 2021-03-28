#include "d_command.h"

#include <array>

#include "d_logger.h"


// CommandPool
namespace dal {

    void CommandPool::init(const uint32_t queue_family_index, const VkDevice logi_device) {
        this->destroy(logi_device);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queue_family_index;
        poolInfo.flags = 0;

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


namespace dal {

    void CmdPoolManager::init(const uint32_t swapchain_count, const uint32_t queue_family_index, const VkDevice logi_device) {
        this->destroy(logi_device);

        this->m_pool_for_single_time.init(queue_family_index, logi_device);
        this->m_pools.resize(swapchain_count);
        this->m_cmd_buffers.resize(swapchain_count);

        for (size_t i = 0; i < this->m_pools.size(); ++i) {
            this->m_pools[i].init(queue_family_index, logi_device);
            this->m_pools[i].allocate(&this->m_cmd_buffers[i], 1, logi_device);
        }
    }

    void CmdPoolManager::destroy(const VkDevice logi_device) {
        this->m_pool_for_single_time.destroy(logi_device);
        for (auto& pool : this->m_pools) {
            pool.destroy(logi_device);
        }
        this->m_pools.clear();
    }

    void CmdPoolManager::record_all_simple(
        const std::vector<VkFramebuffer>& swapchain_fbufs,
        const VkExtent2D& swapchain_extent,
        const VkBuffer vertex_buffer,
        const VkBuffer index_buffer,
        const uint32_t index_size,
        const VkRenderPass render_pass,
        const VkPipeline graphics_pipeline
    ) {
        for (size_t i = 0; i < this->m_cmd_buffers.size(); ++i) {
            auto& cmd_buf = this->m_cmd_buffers[i];

            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = 0;
            begin_info.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(cmd_buf, &begin_info) != VK_SUCCESS) {
                dalAbort("failed to begin recording command buffer!");
            }

            std::array<VkClearValue, 1> clear_colors{
                { 0, 0, 0, 1 },
            };

            VkRenderPassBeginInfo render_pass_info{};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass = render_pass;
            render_pass_info.framebuffer = swapchain_fbufs[i];
            render_pass_info.renderArea.offset = {0, 0};
            render_pass_info.renderArea.extent = swapchain_extent;
            render_pass_info.clearValueCount = clear_colors.size();
            render_pass_info.pClearValues = clear_colors.data();

            vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

                std::array<VkBuffer, 1> vert_bufs{ vertex_buffer };
                std::array<VkDeviceSize, 1> vert_offsets{ 0 };
                vkCmdBindVertexBuffers(cmd_buf, 0, vert_bufs.size(), vert_bufs.data(), vert_offsets.data());
                vkCmdBindIndexBuffer(cmd_buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd_buf, index_size, 1, 0, 0, 0);
                //vkCmdDraw(cmd_buf, index_size, 1, 0, 0);
            }
            vkCmdEndRenderPass(cmd_buf);

            if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
                dalAbort("failed to record command buffer!");
            }
        }
    }

}
