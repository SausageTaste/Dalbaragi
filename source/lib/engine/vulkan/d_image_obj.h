#pragma once

#include <queue>
#include <memory>
#include <unordered_map>

#include "dal/util/image_parser.h"
#include "dal/util/filesystem.h"
#include "dal/util/task_thread.h"
#include "d_renderer.h"
#include "d_vulkan_header.h"
#include "d_command.h"


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


    class ISampler {

    private:
        VkSampler m_sampler = VK_NULL_HANDLE;

    protected:
        void create(const VkSamplerCreateInfo& info, const VkDevice logi_device);

    public:
        void destroy(const VkDevice logi_device);

        auto get() const {
            return this->m_sampler;
        }

    };

    class SamplerTexture : public ISampler {

    public:
        void init(
            const bool enable_anisotropy,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

    };

    class SamplerDepth : public ISampler {

    public:
        void init(
            const bool enable_anisotropy,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

    };


    class TextureUnit {

    private:
        TextureImage m_image;
        ImageView m_view;

    public:
        ~TextureUnit();

        bool init(
            dal::CommandPool& cmd_pool,
            const dal::ImageData& img_data,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        bool is_ready() const;

        auto& view() const {
            return this->m_view;
        }

    };


    class TextureProxy : public ITexture {

    private:
        TextureUnit m_texture;

        dal::CommandPool* m_cmd_pool       = nullptr;
        VkQueue           m_graphics_queue = VK_NULL_HANDLE;
        VkPhysicalDevice  m_phys_device    = VK_NULL_HANDLE;
        VkDevice          m_logi_devic     = VK_NULL_HANDLE;

    public:
        void give_dependencies(
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void clear_dependencies();

        bool are_dependencies_ready() const;

        auto raw_view() const {
            return this->m_texture.view().get();
        }

        bool set_image(const dal::ImageData& img_data) override;

        void destroy() override;

        bool is_ready() const override;

    };


    inline auto& handle_cast(dal::ITexture& handle) {
        return dynamic_cast<TextureProxy&>(handle);
    }

    inline auto& handle_cast(const dal::ITexture& handle) {
        return dynamic_cast<const TextureProxy&>(handle);
    }

    inline auto& handle_cast(dal::HTexture& handle) {
        return dynamic_cast<TextureProxy&>(*handle.get());
    }

    inline auto& handle_cast(const dal::HTexture& handle) {
        return dynamic_cast<const TextureProxy&>(*handle.get());
    }


    class SamplerManager {

    private:
        SamplerTexture m_tex_sampler;
        SamplerDepth m_depth_sampler;

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

        auto& sampler_depth() const {
            return this->m_depth_sampler;
        }

    };

}
