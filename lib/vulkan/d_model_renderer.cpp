#include "d_model_renderer.h"


// ModelRenderer
namespace dal {

    void ModelRenderer::init(
        const dal::ModelStatic& model_data,
        dal::CommandPool& cmd_pool,
        TextureManager& tex_man,
        const VkDescriptorSetLayout layout_per_material,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_desc_pool.init(
            1 * model_data.m_units.size(),
            1 * model_data.m_units.size(),
            1,
            1 * model_data.m_units.size(),
            logi_device
        );

        for (auto& unit_data : model_data.m_units) {
            auto& unit = this->m_units.emplace_back();

            unit.m_vert_buffer.init(
                unit_data.m_vertices,
                unit_data.m_indices,
                cmd_pool,
                graphics_queue,
                phys_device,
                logi_device
            );

            unit.m_ubuf.init(phys_device, logi_device);
            dal::U_PerMaterial ubuf_data;
            ubuf_data.m_roughness = unit_data.m_material.m_roughness;
            ubuf_data.m_metallic = unit_data.m_material.m_metallic;
            unit.m_ubuf.copy_to_buffer(ubuf_data, logi_device);

            unit.m_desc_set = this->m_desc_pool.allocate(layout_per_material, logi_device);
            unit.m_desc_set.record_material(
                unit.m_ubuf,
                tex_man.request_asset_tex(unit_data.m_material.m_albedo_map.c_str()).m_view.get(),
                tex_man.sampler_tex().get(),
                logi_device
            );
        }
    }

    void ModelRenderer::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_units) {
            x.m_vert_buffer.destroy(logi_device);
            x.m_ubuf.destroy(logi_device);
        }
        this->m_units.clear();
        this->m_desc_pool.destroy(logi_device);
    }

}
