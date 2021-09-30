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

    private:
        VkFormat m_format_color = VK_FORMAT_UNDEFINED;
        VkFormat m_format_depth = VK_FORMAT_UNDEFINED;
        VkFormat m_format_albedo = VK_FORMAT_UNDEFINED;
        VkFormat m_format_materials = VK_FORMAT_UNDEFINED;
        VkFormat m_format_normal = VK_FORMAT_UNDEFINED;

    public:
        void init(
            const VkFormat format_color,
            const VkFormat format_depth,
            const VkFormat format_albedo,
            const VkFormat format_materials,
            const VkFormat format_normal,
            const VkDevice logi_device
        );

        auto format_color() const {
            return this->m_format_color;
        }

        auto format_depth() const {
            return this->m_format_depth;
        }

        auto format_albedo() const {
            return this->m_format_albedo;
        }

        auto format_materials() const {
            return this->m_format_materials;
        }

        auto format_normal() const {
            return this->m_format_normal;
        }

    };


    class RenderPass_Final : public IRenderPass {

    private:
        VkFormat m_format_swapchain = VK_FORMAT_UNDEFINED;

    public:
        void init(const VkFormat swapchain_img_format, const VkDevice logi_device);

        auto format_swapchain() const {
            return this->m_format_swapchain;
        }

    };


    class RenderPass_Alpha : public IRenderPass {

    private:
        VkFormat m_format_color = VK_FORMAT_UNDEFINED;
        VkFormat m_format_depth = VK_FORMAT_UNDEFINED;

    public:
        void init(
            const VkFormat format_color,
            const VkFormat format_depth,
            const VkDevice logi_device
        );

        auto format_color() const {
            return this->m_format_color;
        }

        auto format_depth() const {
            return this->m_format_depth;
        }

    };


    class RenderPass_ShadowMap : public IRenderPass {

    private:
        VkFormat m_format_shadow_map = VK_FORMAT_UNDEFINED;

    public:
        void init(
            const VkFormat format_shadow_map,
            const VkDevice logi_device
        );

        auto format_shadow_map() const {
            return this->m_format_shadow_map;
        }

    };


    class RenderPass_Simple : public IRenderPass {

    private:
        VkFormat m_format_color = VK_FORMAT_UNDEFINED;
        VkFormat m_format_depth = VK_FORMAT_UNDEFINED;

    public:
        void init(
            const VkFormat format_color,
            const VkFormat format_depth,
            const VkDevice logi_device
        );

        auto format_color() const {
            return this->m_format_color;
        }

        auto format_depth() const {
            return this->m_format_depth;
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
        RenderPass_Gbuf m_rp_gbuf;
        RenderPass_Final m_rp_final;
        RenderPass_Alpha m_rp_alpha;
        RenderPass_ShadowMap m_rp_shadow;
        RenderPass_Simple m_rp_simple;

    public:
        void init(
            const VkFormat format_swapchain,
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

        auto& rp_alpha() const {
            return this->m_rp_alpha;
        }

        auto& rp_shadow() const {
            return this->m_rp_shadow;
        }

        auto& rp_simple() const {
            return this->m_rp_simple;
        }

    };

}
