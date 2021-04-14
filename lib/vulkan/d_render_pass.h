#pragma once

#include <array>

#include "d_vulkan_header.h"


namespace dal {

    class RenderPass {

    private:
        VkRenderPass m_handle = VK_NULL_HANDLE;

    public:
        RenderPass() = default;

        RenderPass(const RenderPass&) = delete;
        RenderPass& operator=(const RenderPass&) = delete;
        RenderPass(RenderPass&&) = delete;
        RenderPass& operator=(RenderPass&&) = delete;

    public:
        ~RenderPass();

        void init(const VkRenderPass handle) {
            this->m_handle = handle;
        }

        void destroy(const VkDevice logi_device);

        void operator=(const VkRenderPass handle) {
            this->init(handle);
        }

        VkRenderPass get() const {
            return this->m_handle;
        }

    };


    class RenderPassManager {

    public:
        RenderPassManager() = default;

        RenderPassManager(const RenderPassManager&) = delete;
        RenderPassManager& operator=(const RenderPassManager&) = delete;
        RenderPassManager(RenderPassManager&&) = delete;
        RenderPassManager& operator=(RenderPassManager&&) = delete;

    private:
        RenderPass m_rp_rendering;

    public:
        void init(
            const VkFormat format_color,
            const VkFormat format_depth,
            const VkDevice logi_device
        );

        void destroy(VkDevice logi_device);

        auto& rp_rendering(void) const {
            return this->m_rp_rendering;
        }

    };

}
