#pragma once

#include <memory>
#include <unordered_map>

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

        auto get() const {
            return this->m_view;
        }

    };


    class TextureImage {

    private:
        VkImage m_image = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

        VkFormat m_format;

    public:
        void init_texture(
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


    class TextureUnit {

    private:
        TextureImage m_image;
        ImageView m_view;

    public:
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


    class TextureManager : public ITaskListener {

    private:
        std::unordered_map<std::string, TextureUnit> m_textures;
        std::unordered_map<void*, TextureUnit*> m_sent_task;
        Sampler m_tex_sampler;

        dal::TaskManager* m_task_man = nullptr;
        Filesystem* m_filesys = nullptr;
        CommandPool* m_cmd_pool = nullptr;
        VkQueue m_graphics_queue = VK_NULL_HANDLE;
        VkPhysicalDevice m_phys_device = VK_NULL_HANDLE;
        VkDevice m_logi_device = VK_NULL_HANDLE;

    public:
        void init(
            dal::TaskManager& task_man,
            dal::Filesystem& filesys,
            CommandPool& cmd_pool,
            const bool enable_anisotropy,
            const VkQueue m_graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        void notify_task_done(std::unique_ptr<ITask> task) override;

        const TextureUnit& request_asset_tex(const dal::ResPath& path);

        const TextureUnit& get_missing_tex();

        auto& sampler_tex() const {
            return this->m_tex_sampler;
        }

    };

}
