#include "d_sync_primitives.h"

#include "d_logger.h"


// Semaphore
namespace dal {

    Semaphore::Semaphore(Semaphore&& other) {
        std::swap(this->m_handle, other.m_handle);
    }

    Semaphore& Semaphore::operator=(Semaphore&& other) {
        std::swap(this->m_handle, other.m_handle);
        return *this;
    }

    void Semaphore::init(const VkDevice logi_device) {
        this->destory(logi_device);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (VK_SUCCESS != vkCreateSemaphore(logi_device, &semaphore_info, nullptr, &this->m_handle)) {
            dalAbort("failed to create a semaphore");
        }
    }

    void Semaphore::destory(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_handle) {
            vkDestroySemaphore(logi_device, this->m_handle, nullptr);
            this->m_handle = VK_NULL_HANDLE;
        }
    }

}


// Fence
namespace dal {

    Fence::Fence(Fence&& other) {
        std::swap(this->m_handle, other.m_handle);
    }

    Fence& Fence::operator=(Fence&& other) {
        std::swap(this->m_handle, other.m_handle);
        return *this;
    }

    void Fence::init(const VkDevice logi_device) {
        this->destory(logi_device);

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (VK_SUCCESS != vkCreateFence(logi_device, &fence_info, nullptr, &this->m_handle)) {
            dalAbort("failed to create a fence");
        }
    }

    void Fence::destory(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_handle) {
            vkDestroyFence(logi_device, this->m_handle, nullptr);
            this->m_handle = VK_NULL_HANDLE;
        }
    }

    void Fence::wait(const VkDevice logi_device) const {
        vkWaitForFences(logi_device, 1, &this->m_handle, VK_TRUE, UINT64_MAX);
    }

    void Fence::reset(const VkDevice logi_device) const {
        vkResetFences(logi_device, 1, &this->m_handle);
    }

}
