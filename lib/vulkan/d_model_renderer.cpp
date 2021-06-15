#include "d_model_renderer.h"

#include <fmt/format.h>

#include "d_indices.h"


// ActorVK
namespace dal {

    ActorVK::~ActorVK() {
        this->destroy();
    }

    void ActorVK::init(
        DescPool& desc_pool,
        const VkDescriptorSetLayout layout_per_actor,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy();

        this->m_logi_device = logi_device;
        this->m_ubuf_per_actor.init(phys_device, logi_device);
        this->m_desc_per_actor = desc_pool.allocate(layout_per_actor, logi_device);
        this->m_desc_per_actor.record_per_actor(this->m_ubuf_per_actor, logi_device);
    }

    void ActorVK::destroy() {
        this->m_ubuf_per_actor.destroy(this->m_logi_device);
        this->m_logi_device = VK_NULL_HANDLE;
    }

    bool ActorVK::is_ready() const {
        return this->m_desc_per_actor.is_ready() && this->m_ubuf_per_actor.is_ready();
    }

    void ActorVK::apply_changes() {
        if (this->is_ready()) {
            U_PerActor ubuf_data_per_actor;
            ubuf_data_per_actor.m_model = this->m_transform.make_mat4();
            this->m_ubuf_per_actor.copy_to_buffer(ubuf_data_per_actor, this->m_logi_device);
        }
    }

}


// ActorSkinnedVK
namespace dal {

    ActorSkinnedVK::~ActorSkinnedVK() {
        this->destroy();
    }

    void ActorSkinnedVK::init(
        DescPool& desc_pool,
        const VkDescriptorSetLayout layout_per_actor,
        const VkDescriptorSetLayout layout_anim,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy();

        this->m_logi_device = logi_device;

        this->m_ubuf_per_actor.init(dal::MAX_FRAMES_IN_FLIGHT, phys_device, logi_device);
        for (int i = 0; i < dal::MAX_FRAMES_IN_FLIGHT; ++i) {
            this->m_desc_per_actor.push_back(desc_pool.allocate(layout_per_actor, logi_device));
            this->m_desc_per_actor.back().record_per_actor(this->m_ubuf_per_actor.at(i), logi_device);
        }

        this->m_ubuf_anim.init(dal::MAX_FRAMES_IN_FLIGHT, phys_device, logi_device);
        for (int i = 0; i < dal::MAX_FRAMES_IN_FLIGHT; ++i) {
            this->m_desc_animation.push_back(desc_pool.allocate(layout_anim, logi_device));
            this->m_desc_animation.back().record_animation(this->m_ubuf_anim.at(i), logi_device);
        }
    }

    void ActorSkinnedVK::destroy() {
        this->m_ubuf_per_actor.destroy(this->m_logi_device);
        this->m_ubuf_anim.destroy(this->m_logi_device);
        this->m_logi_device = VK_NULL_HANDLE;
    }

    bool ActorSkinnedVK::is_ready() const {
        return VK_NULL_HANDLE != this->m_logi_device;
    }

    void ActorSkinnedVK::apply_transform(const FrameInFlightIndex& index) {
        if (!this->is_ready())
            return;

        U_PerActor ubuf_data_per_actor;
        ubuf_data_per_actor.m_model = this->m_transform.make_mat4();
        this->m_ubuf_per_actor.copy_to_buffer(index.get(), ubuf_data_per_actor, this->m_logi_device);
    }

    void ActorSkinnedVK::apply_animation(const FrameInFlightIndex& index) {
        if (!this->is_ready())
            return;

        U_AnimTransform ubuf_data;

        const auto size = std::min<uint32_t>(MAX_JOINT_SIZE, this->m_anim_state.getTransformArray().getSize());
        for (uint32_t i = 0; i < size; ++i)
            ubuf_data.m_transforms[i] = this->m_anim_state.getTransformArray().list().at(i);

        this->m_ubuf_anim.copy_to_buffer(index.get(), ubuf_data, this->m_logi_device);
    }

}


// RenderUnit
namespace dal {

    void RenderUnit::init_static(
        const dal::RenderUnitStatic& unit_data,
        dal::CommandPool& cmd_pool,
        ITextureManager& tex_man,
        const char* const fallback_file_namespace,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        auto& unit = *this;

        unit.m_weight_center = unit_data.m_weight_center;

        unit.m_vert_buffer.init_static(
            unit_data.m_vertices,
            unit_data.m_indices,
            cmd_pool,
            graphics_queue,
            phys_device,
            logi_device
        );

        unit.m_material.m_alpha_blend = unit_data.m_material.m_alpha_blending;
        unit.m_material.m_data.m_roughness = unit_data.m_material.m_roughness;
        unit.m_material.m_data.m_metallic = unit_data.m_material.m_metallic;
        unit.m_material.m_ubuf.init(phys_device, logi_device);
        unit.m_material.m_ubuf.copy_to_buffer(unit.m_material.m_data, logi_device);

        const auto albedo_map_path = fmt::format("{}/?/{}", fallback_file_namespace, unit_data.m_material.m_albedo_map);
        unit.m_material.m_albedo_map = tex_man.request_texture(albedo_map_path);
    }

    void RenderUnit::init_skinned(
        const dal::RenderUnitSkinned& unit_data,
        dal::CommandPool& cmd_pool,
        ITextureManager& tex_man,
        const char* const fallback_file_namespace,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        auto& unit = *this;

        unit.m_weight_center = unit_data.m_weight_center;

        unit.m_vert_buffer.init_skinned(
            unit_data.m_vertices,
            unit_data.m_indices,
            cmd_pool,
            graphics_queue,
            phys_device,
            logi_device
        );

        unit.m_material.m_alpha_blend = unit_data.m_material.m_alpha_blending;
        unit.m_material.m_data.m_roughness = unit_data.m_material.m_roughness;
        unit.m_material.m_data.m_metallic = unit_data.m_material.m_metallic;
        unit.m_material.m_ubuf.init(phys_device, logi_device);
        unit.m_material.m_ubuf.copy_to_buffer(unit.m_material.m_data, logi_device);

        const auto albedo_map_path = fmt::format("{}/?/{}", fallback_file_namespace, unit_data.m_material.m_albedo_map);
        unit.m_material.m_albedo_map = tex_man.request_texture(albedo_map_path);
    }

    void RenderUnit::destroy(const VkDevice logi_device) {
        this->m_material.m_ubuf.destroy(logi_device);
        this->m_vert_buffer.destroy(logi_device);
    }

    bool RenderUnit::prepare(
        DescPool& desc_pool,
        const VkSampler sampler,
        const VkDescriptorSetLayout layout_per_material,
        const VkDevice logi_device
    ) {
        if (this->is_ready())
            return true;
        if (!this->m_material.m_albedo_map->is_ready())
            return false;

        this->m_material.m_descset = desc_pool.allocate(layout_per_material, logi_device);

        auto albedo_map = reinterpret_cast<TextureUnit*>(this->m_material.m_albedo_map.get());

        this->m_material.m_descset.record_material(
            this->m_material.m_ubuf,
            albedo_map->view().get(),
            sampler,
            logi_device
        );

        return true;
    }

    bool RenderUnit::is_ready() const {
        if (!this->m_material.m_descset.is_ready())
            return false;

        return true;
    }

}


// ModelRenderer
namespace dal {

    void ModelRenderer::init(
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy();
        this->m_logi_device = logi_device;
    }

    void ModelRenderer::upload_meshes(
        const dal::ModelStatic& model_data,
        dal::CommandPool& cmd_pool,
        ITextureManager& tex_man,
        const char* const fallback_file_namespace,
        const VkDescriptorSetLayout layout_per_actor,
        const VkDescriptorSetLayout layout_per_material,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_desc_pool.init(
            1 * model_data.m_units.size() + 5,
            1 * model_data.m_units.size() + 5,
            5,
            1 * model_data.m_units.size() + 5,
            logi_device
        );

        for (auto& unit_data : model_data.m_units) {
            auto& unit = unit_data.m_material.m_alpha_blending ? this->m_units_alpha.emplace_back() : this->m_units.emplace_back();

            unit.init_static(
                unit_data,
                cmd_pool,
                tex_man,
                fallback_file_namespace,
                graphics_queue,
                phys_device,
                logi_device
            );
        }
    }

    void ModelRenderer::destroy() {
        for (auto& x : this->m_units)
            x.destroy(this->m_logi_device);
        this->m_units.clear();

        for (auto& x : this->m_units_alpha)
            x.destroy(this->m_logi_device);
        this->m_units_alpha.clear();

        this->m_desc_pool.destroy(this->m_logi_device);
    }

    bool ModelRenderer::fetch_one_resource(const VkDescriptorSetLayout layout_per_material, const VkSampler sampler, const VkDevice logi_device) {
        for (auto& unit : this->m_units) {
            if (unit.is_ready())
                continue;
            if (unit.prepare(this->m_desc_pool, sampler, layout_per_material, logi_device))
                return true;
        }

        for (auto& unit : this->m_units_alpha) {
            if (unit.is_ready())
                continue;
            if (unit.prepare(this->m_desc_pool, sampler, layout_per_material, logi_device))
                return true;
        }

        return false;
    }

    bool ModelRenderer::is_ready() const {
        if (this->m_units.empty() && this->m_units_alpha.empty())
            return false;

        for (auto& unit : this->m_units) {
            if (!unit.is_ready())
                return false;
        }

        for (auto& unit : this->m_units_alpha) {
            if (!unit.is_ready())
                return false;
        }

        return true;
    }

}


// ModelRenderer
namespace dal {

    void ModelSkinnedRenderer::init(
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy();
        this->m_logi_device = logi_device;
    }

    void ModelSkinnedRenderer::upload_meshes(
        const dal::ModelSkinned& model_data,
        dal::CommandPool& cmd_pool,
        ITextureManager& tex_man,
        const char* const fallback_file_namespace,
        const VkDescriptorSetLayout layout_per_actor,
        const VkDescriptorSetLayout layout_per_material,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_desc_pool.init(
            1 * model_data.m_units.size() + 5,
            1 * model_data.m_units.size() + 5,
            5,
            1 * model_data.m_units.size() + 5,
            logi_device
        );

        for (auto& unit_data : model_data.m_units) {
            auto& unit = unit_data.m_material.m_alpha_blending ? this->m_units_alpha.emplace_back() : this->m_units.emplace_back();

            unit.init_skinned(
                unit_data,
                cmd_pool,
                tex_man,
                fallback_file_namespace,
                graphics_queue,
                phys_device,
                logi_device
            );
        }

        this->m_animations = model_data.m_animations;
        this->m_skeleton_interf = model_data.m_skeleton;
    }

    void ModelSkinnedRenderer::destroy() {
        for (auto& x : this->m_units)
            x.destroy(this->m_logi_device);
        this->m_units.clear();

        for (auto& x : this->m_units_alpha)
            x.destroy(this->m_logi_device);
        this->m_units_alpha.clear();

        this->m_desc_pool.destroy(this->m_logi_device);
    }

    bool ModelSkinnedRenderer::fetch_one_resource(const VkDescriptorSetLayout layout_per_material, const VkSampler sampler, const VkDevice logi_device) {
        for (auto& unit : this->m_units) {
            if (unit.is_ready())
                continue;
            if (unit.prepare(this->m_desc_pool, sampler, layout_per_material, logi_device))
                return true;
        }

        for (auto& unit : this->m_units_alpha) {
            if (unit.is_ready())
                continue;
            if (unit.prepare(this->m_desc_pool, sampler, layout_per_material, logi_device))
                return true;
        }

        return false;
    }

    bool ModelSkinnedRenderer::is_ready() const {
        if (this->m_units.empty() && this->m_units_alpha.empty())
            return false;

        for (auto& unit : this->m_units) {
            if (!unit.is_ready())
                return false;
        }

        for (auto& unit : this->m_units_alpha) {
            if (!unit.is_ready())
                return false;
        }

        return true;
    }

}
