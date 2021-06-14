#pragma once

#include <queue>
#include <memory>
#include <unordered_map>

#include "d_renderer.h"
#include "d_vulkan_header.h"
#include "d_image_parser.h"
#include "d_command.h"
#include "d_filesystem.h"
#include "d_task_thread.h"


namespace dal {

    class ImageView {

    private:
        VkImageView m_view = VK_NULL_HANDLE;

    public:
        ImageView() = default;
        ImageView(const ImageView&) = delete;
        ImageView& operator=(const ImageView&) = delete;

    public:
        ImageView(
            const VkImage image,
            const VkFormat format,
            const uint32_t mip_level,
            const VkImageAspectFlags aspect_flags,
            const VkDevice logi_device
        );

        ~ImageView();

        ImageView(ImageView&&) noexcept;

        ImageView& operator=(ImageView&&) noexcept;

        [[nodiscard]]
        bool init(
            const VkImage image,
            const VkFormat format,
            const uint32_t mip_level,
            const VkImageAspectFlags aspect_flags,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        bool is_ready() const {
            return VK_NULL_HANDLE != this->m_view;
        }

        VkImageView get() const;

    };


    class TextureImage {

    private:
        VkImage m_image = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

        VkFormat m_format;
        uint32_t m_mip_levels = 1;

    public:
        void init_texture(
            const ImageData& img,
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void init_texture_gen_mipmaps(
            const ImageData& img,
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void init_attachment(
            const uint32_t width,
            const uint32_t height,
            const VkFormat format,
            const VkImageUsageFlags usage_flags,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destory(const VkDevice logi_device);

        bool is_ready() const;

        auto image() const {
            return this->m_image;
        }

        auto format() const {
            return this->m_format;
        }

        auto mip_levels() const {
            return this->m_mip_levels;
        }

    };


    class Sampler {

    private:
        VkSampler m_sampler = VK_NULL_HANDLE;

    public:
        void init_for_color_map(
            const bool enable_anisotropy,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto get() const {
            return this->m_sampler;
        }

    };


    class TextureUnit : public ITexture {

    private:
        TextureImage m_image;
        ImageView m_view;
        VkDevice m_logi_device = VK_NULL_HANDLE;

    public:
        ~TextureUnit() override;

        bool init(
            dal::CommandPool& cmd_pool,
            const dal::ImageData& img_data,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy();

        bool is_ready() const override;

        auto& view() const {
            return this->m_view;
        }

    };


    class SamplerManager {

    private:
        Sampler m_tex_sampler;

    public:
        void init(
            const bool enable_anisotropy,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& sampler_tex() const {
            return this->m_tex_sampler;
        }

    };

}
