#pragma once

#include "d_renderer.h"
#include "d_vert_data.h"
#include "d_uniform.h"
#include "d_image_obj.h"
#include "d_filesystem.h"


namespace dal {

    class ActorVK : public IActor {

    private:
        DescSet m_desc_per_actor;
        dal::UniformBuffer<dal::U_PerActor> m_ubuf_per_actor;
        VkDevice m_logi_device = VK_NULL_HANDLE;

    public:
        ~ActorVK();

        void init(
            DescPool& desc_pool,
            const VkDescriptorSetLayout layout_per_actor,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy();

        void apply_changes() override;

        auto& desc_set_raw() const {
            return this->m_desc_per_actor.get();
        }

    };


    class RenderUnit {

    private:
        struct Material {
            U_PerMaterial m_data;
            UniformBuffer<U_PerMaterial> m_ubuf;
            DescSet m_descset;
            std::shared_ptr<ITexture> m_albedo_map;
            bool m_alpha_blend = false;
        };

    public:
        Material m_material;
        VertexBuffer m_vert_buffer;
        glm::vec3 m_weight_center{ 0 };

    public:
        void init(
            const dal::RenderUnitStatic& unit_data,
            dal::CommandPool& cmd_pool,
            TextureManager& tex_man,
            const char* const fallback_file_namespace,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        bool prepare(
            DescPool& desc_pool,
            const VkSampler sampler,
            const VkDescriptorSetLayout layout_per_material,
            const VkDevice logi_device
        );

        bool is_ready() const;

    };


    class ModelRenderer : public IRenModel {

    private:
        std::vector<RenderUnit> m_units;
        std::vector<RenderUnit> m_units_alpha;
        DescPool m_desc_pool;

        VkDevice m_logi_device = VK_NULL_HANDLE;

    public:
        ~ModelRenderer() override {
            this->destroy();
        }

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

        void destroy();

        bool fetch_one_resource(const VkDescriptorSetLayout layout_per_material, const VkSampler sampler, const VkDevice logi_device);

        bool is_ready() const override;

        auto& render_units() const {
            return this->m_units;
        }

        auto& render_units_alpha() const {
            return this->m_units_alpha;
        }

    };


    class ModelManager : public ITaskListener {

    private:
        std::unordered_map<std::string, HRenModel> m_models;
        std::unordered_map<void*, ModelRenderer*> m_sent_task;
        dal::CommandPool m_cmd_pool;

        dal::TaskManager* m_task_man;
        dal::Filesystem* m_filesys;
        dal::TextureManager* m_tex_man;
        dal::DescSetLayoutManager* m_desc_layout_man;
        VkQueue m_graphics_queue;
        VkPhysicalDevice m_phys_device;
        VkDevice m_logi_device;

    public:
        void init(
            dal::TaskManager& task_man,
            dal::Filesystem& filesys,
            dal::TextureManager& tex_man,
            dal::DescSetLayoutManager& desc_layout_man,
            const uint32_t queue_family_index,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        ) {
            m_task_man = &task_man;
            m_filesys = &filesys;
            m_tex_man = &tex_man;
            m_desc_layout_man = &desc_layout_man;
            m_graphics_queue = graphics_queue;
            m_phys_device = phys_device;
            m_logi_device = logi_device;

            this->m_cmd_pool.init(queue_family_index, logi_device);
        }

        void destroy(const VkDevice logi_device);

        void update();

        void notify_task_done(std::unique_ptr<ITask> task) override;

        HRenModel request_model(const dal::ResPath& respath);

    };

}
