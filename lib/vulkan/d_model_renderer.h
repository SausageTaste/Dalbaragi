#pragma once

#include "d_vert_data.h"
#include "d_uniform.h"
#include "d_image_obj.h"


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
            const dal::ModelStatic& model_data,
            dal::CommandPool& cmd_pool,
            TextureManager& tex_man,
            const VkDescriptorSetLayout layout_per_material,
            const VkDescriptorSetLayout layout_per_actor,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

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

}
