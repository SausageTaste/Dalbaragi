#pragma once

#include "d_vert_data.h"
#include "d_uniform.h"


namespace dal {

    class ModelRenderer {

        struct RenderUnit {
            dal::VertexBuffer m_vert_buffer;
            dal::UniformBuffer<dal::U_PerMaterial> m_ubuf;
            dal::DescSet m_desc_set;
        };

    private:
        std::vector<RenderUnit> m_units;

    public:
        void init(
            const dal::ModelStatic& model_data,
            dal::CommandPool& cmd_pool,
            dal::DescPool& desc_pool,
            const VkImageView albedo_map_view,
            const VkSampler sampler,
            const VkDescriptorSetLayout layout_per_material,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& render_units() const {
            return this->m_units;
        }

    };

}
