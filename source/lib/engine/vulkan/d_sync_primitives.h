#pragma once

#include "d_vulkan_header.h"


namespace dal {

    class Semaphore {

    private:
        VkSemaphore m_handle = VK_NULL_HANDLE;

    public:
        Semaphore() = default;

        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

    public:
        Semaphore(Semaphore&& other);
        Semaphore& operator=(Semaphore&& other);

        void init(const VkDevice logi_device);

        void destory(const VkDevice logi_device);

        auto get() const {
            return this->m_handle;
        }

    };


    class Fence {

    private:
        VkFence m_handle = VK_NULL_HANDLE;

    public:
        Fence() = default;

        Fence(const Fence&) = delete;
        Fence& operator=(const Fence&) = delete;

    public:
        Fence(Fence&& other);

        Fence& operator=(Fence&& other);

        void init(const VkDevice logi_device);

        void destory(const VkDevice logi_device);

        auto get() const {
            return this->m_handle;
        }

        void wait(const VkDevice logi_device) const;

        void reset(const VkDevice logi_device) const;

        void wait_reset(const VkDevice logi_device) const {
            this->wait(logi_device);
            this->reset(logi_device);
        }

    };

}
