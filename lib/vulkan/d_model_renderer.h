#pragma once

#include "d_vert_data.h"
#include "d_uniform.h"
#include "d_image_obj.h"
#include "d_task_thread.h"
#include "d_filesystem.h"


namespace dal {

    class ModelRenderer {

    public:
        struct RenderUnit {
            dal::VertexBuffer m_vert_buffer;
            dal::UniformBuffer<dal::U_PerMaterial> m_ubuf;
            dal::DescSet m_desc_set;
        };

    private:
        std::vector<RenderUnit> m_units;
        DescPool m_desc_pool;

        DescSet m_desc_per_actor;
        dal::UniformBuffer<dal::U_PerActor> m_ubuf_per_actor;

    public:
        void init(
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void upload_meshes(
            const dal::ModelStatic& model_data,
            dal::CommandPool& cmd_pool,
            TextureManager& tex_man,
            const char* const fallback_file_namespace,
            const VkDescriptorSetLayout layout_per_actor,
            const VkDescriptorSetLayout layout_per_material,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        bool is_ready() const;

        auto& render_units() const {
            return this->m_units;
        }

        auto& ubuf_per_actor() {
            return this->m_ubuf_per_actor;
        }

        auto desc_set_per_actor() const {
            return this->m_desc_per_actor;
        }

    };


    class ModelManager : public ITaskListener {

    private:
        std::unordered_map<std::string, ModelRenderer> m_models;
        std::unordered_map<void*, ModelRenderer*> m_sent_task;

        dal::TaskManager* m_task_man;
        dal::Filesystem* m_filesys;
        dal::TextureManager* m_tex_man;
        dal::CommandPool* m_single_time_pool;
        dal::DescSetLayoutManager* m_desc_layout_man;
        VkQueue m_graphics_queue;
        VkPhysicalDevice m_phys_device;
        VkDevice m_logi_device;

    public:
        void init(
            dal::TaskManager& task_man,
            dal::Filesystem& filesys,
            dal::TextureManager& tex_man,
            dal::CommandPool& single_time_pool,
            dal::DescSetLayoutManager& desc_layout_man,
            VkQueue graphics_queue,
            VkPhysicalDevice phys_device,
            VkDevice logi_device
        ) {
            m_task_man = &task_man;
            m_filesys = &filesys;
            m_tex_man = &tex_man;
            m_single_time_pool = &single_time_pool;
            m_desc_layout_man = &desc_layout_man;
            m_graphics_queue = graphics_queue;
            m_phys_device = phys_device;
            m_logi_device = logi_device;
        }

        void destroy(const VkDevice logi_device);

        void notify_task_done(std::unique_ptr<ITask> task) override;

        ModelRenderer& request_model(const dal::ResPath& respath);

    };

}
