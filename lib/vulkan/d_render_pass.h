#pragma once

#include <array>

#include "d_vulkan_header.h"


namespace dal {

    class IRenderPass {

    protected:
        VkRenderPass m_handle = VK_NULL_HANDLE;

    public:
        IRenderPass() = default;

        IRenderPass(const IRenderPass&) = delete;
        IRenderPass& operator=(const IRenderPass&) = delete;

    public:
        IRenderPass(IRenderPass&& other) noexcept;

        IRenderPass& operator=(IRenderPass&& other) noexcept;

        ~IRenderPass();

        void destroy(const VkDevice logi_device);

        bool is_ready() const {
            return VK_NULL_HANDLE != this->m_handle;
        }

        VkRenderPass get() const;

    };


    class RenderPass_Gbuf : public IRenderPass {

    public:
        void init(
            const VkFormat format_color,
            const VkFormat format_depth,
            const VkDevice logi_device
        );

    };


    class RenderPass_Final : public IRenderPass {

    public:
        void init(const VkFormat swapchain_img_format, const VkDevice logi_device);

    };


    class RenderPassManager {

    public:
        RenderPassManager() = default;

        RenderPassManager(const RenderPassManager&) = delete;
        RenderPassManager& operator=(const RenderPassManager&) = delete;
        RenderPassManager(RenderPassManager&&) = delete;
        RenderPassManager& operator=(RenderPassManager&&) = delete;

    private:
        RenderPass_Gbuf m_rp_gbuf;
        RenderPass_Final m_rp_final;

    public:
        void init(
            const VkFormat format_color,
            const VkFormat format_depth,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& rp_gbuf() const {
            return this->m_rp_gbuf;
        }

        auto& rp_final() const {
            return this->m_rp_final;
        }

    };

}
