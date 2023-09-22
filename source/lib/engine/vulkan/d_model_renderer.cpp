#include "d_model_renderer.h"

#include <fmt/format.h>

#include "d_indices.h"


// ActorVK
namespace dal {

    void ActorVK::init(
        DescAllocator& desc_allocator,
        const dal::DescLayout_PerActor& layout_per_actor,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(desc_allocator, logi_device);

        this->m_ubuf_per_actor.init(dal::MAX_FRAMES_IN_FLIGHT, phys_device, logi_device);

        for (int i = 0; i < dal::MAX_FRAMES_IN_FLIGHT; ++i) {
            this->m_desc.push_back(desc_allocator.allocate(layout_per_actor, logi_device));
            this->m_desc.back().record_per_actor(
                this->m_ubuf_per_actor.at(i),
                logi_device
            );
        }

        this->m_transform_update_needed = dal::MAX_FRAMES_IN_FLIGHT;
    }

    void ActorVK::destroy(DescAllocator& desc_allocator, const VkDevice logi_device) {
        for (auto& d : this->m_desc)
            desc_allocator.free(std::move(d));
        this->m_desc.clear();

        this->m_ubuf_per_actor.destroy(logi_device);
    }

    bool ActorVK::is_ready() const {
        for (auto& d : this->m_desc) {
            if (!d.is_ready()) {
                return false;
            }
        }

        return this->m_ubuf_per_actor.is_ready();
    }

    void ActorVK::apply_transform(const FrameInFlightIndex& index, const dal::Transform& transform, const VkDevice logi_device) {
        if (!this->is_ready())
            return;

        U_PerActor ubuf_data_per_actor;
        ubuf_data_per_actor.m_model = transform.make_mat4();
        this->m_ubuf_per_actor.copy_to_buffer(index.get(), ubuf_data_per_actor, logi_device);
    }

}


// ActorProxy
namespace dal {

    ActorProxy::~ActorProxy() {
        this->destroy();
        this->clear_dependencies();
    }

    void ActorProxy::give_dependencies(
        DescAllocator& desc_allocator,
        const DescLayout_PerActor& desc_layout,
        VkPhysicalDevice phys_device,
        VkDevice logi_device
    ) {
        m_desc_allocator = &desc_allocator;
        m_desc_layout = &desc_layout;
        m_phys_device = phys_device;
        m_logi_device = logi_device;
    }

    void ActorProxy::clear_dependencies() {
        m_desc_allocator = nullptr;
        m_desc_layout = nullptr;
        m_phys_device = VK_NULL_HANDLE;;
        m_logi_device = VK_NULL_HANDLE;
    }

    bool ActorProxy::are_dependencies_ready() const {
        return (
            m_desc_allocator != nullptr &&
            m_desc_layout != nullptr &&
            m_phys_device != VK_NULL_HANDLE &&
            m_logi_device != VK_NULL_HANDLE
        );
    }

    void ActorProxy::apply_transform(const FrameInFlightIndex& index) {
        this->get().apply_transform(index, this->m_transform, this->m_logi_device);
    }

    // Overridings

    bool ActorProxy::init() {
        if (!this->are_dependencies_ready())
            return false;

        this->m_actor.init(
            *this->m_desc_allocator,
            *this->m_desc_layout,
            this->m_phys_device,
            this->m_logi_device
        );

        return true;
    }

    void ActorProxy::destroy() {
        this->m_actor.destroy(*this->m_desc_allocator, this->m_logi_device);
    }

    bool ActorProxy::is_ready() const {
        return this->m_actor.is_ready();
    }

    void ActorProxy::notify_transform_change() {
        this->m_actor.notify_transform_change();
    }

}


// ActorSkinnedVK
namespace dal {

    void ActorSkinnedVK::init(
        DescAllocator& desc_allocator,
        const dal::DescLayout_ActorAnimated& layout_per_actor,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(desc_allocator, logi_device);

        this->m_ubuf_per_actor.init(dal::MAX_FRAMES_IN_FLIGHT, phys_device, logi_device);
        this->m_ubuf_anim.init(dal::MAX_FRAMES_IN_FLIGHT, phys_device, logi_device);

        for (int i = 0; i < dal::MAX_FRAMES_IN_FLIGHT; ++i) {
            this->m_desc.push_back(desc_allocator.allocate(layout_per_actor, logi_device));

            this->m_desc.back().record_actor_animated(
                this->m_ubuf_per_actor.at(i),
                this->m_ubuf_anim.at(i),
                logi_device
            );
        }
    }

    void ActorSkinnedVK::destroy(DescAllocator& desc_allocator, const VkDevice logi_device) {
        for (auto& d : this->m_desc)
            desc_allocator.free(std::move(d));
        this->m_desc.clear();

        this->m_ubuf_per_actor.destroy(logi_device);
        this->m_ubuf_anim.destroy(logi_device);
    }

    bool ActorSkinnedVK::is_ready() const {
        return (
            this->m_ubuf_per_actor.is_ready() &&
            this->m_ubuf_anim.is_ready()
        );
    }

    void ActorSkinnedVK::apply_transform(const FrameInFlightIndex& index, const dal::Transform& transform, const VkDevice logi_device) {
        if (!this->is_ready())
            return;

        U_PerActor ubuf_data_per_actor;
        ubuf_data_per_actor.m_model = transform.make_mat4();
        this->m_ubuf_per_actor.copy_to_buffer(index.get(), ubuf_data_per_actor, logi_device);
    }

    void ActorSkinnedVK::apply_animation(const FrameInFlightIndex& index, const dal::AnimationState& anim_state, const VkDevice logi_device) {
        if (!this->is_ready())
            return;

        U_AnimTransform ubuf_data;

        const auto size = std::min<uint32_t>(dal::MAX_JOINT_COUNT, anim_state.transform_array().size());
        for (uint32_t i = 0; i < size; ++i)
            ubuf_data.m_transforms[i] = anim_state.transform_array()[i];

        this->m_ubuf_anim.copy_to_buffer(index.get(), ubuf_data, logi_device);
    }

}


// ActorSkinnedProxy
namespace dal {

    ActorSkinnedProxy::~ActorSkinnedProxy() {
        this->destroy();
        this->clear_dependencies();
    }

    void ActorSkinnedProxy::give_dependencies(
        DescAllocator& desc_allocator,
        const DescLayout_ActorAnimated& desc_layout,
        VkPhysicalDevice phys_device,
        VkDevice logi_device
    ) {
        m_desc_allocator = &desc_allocator;
        m_desc_layout = &desc_layout;
        m_phys_device = phys_device;
        m_logi_device = logi_device;
    }

    void ActorSkinnedProxy::clear_dependencies() {
        m_desc_allocator = nullptr;
        m_desc_layout = nullptr;
        m_phys_device = VK_NULL_HANDLE;;
        m_logi_device = VK_NULL_HANDLE;
    }

    bool ActorSkinnedProxy::are_dependencies_ready() const {
        return (
            m_desc_allocator != nullptr &&
            m_desc_layout != nullptr &&
            m_phys_device != VK_NULL_HANDLE &&
            m_logi_device != VK_NULL_HANDLE
        );
    }

    void ActorSkinnedProxy::apply_transform(const FrameInFlightIndex& index) {
        this->m_actor.apply_transform(index, this->m_transform, this->m_logi_device);
    }

    void ActorSkinnedProxy::apply_animation(const FrameInFlightIndex& index) {
        this->m_actor.apply_animation(index, this->m_anim_state, this->m_logi_device);
    }

    // Overridings

    bool ActorSkinnedProxy::init() {
        if (!this->are_dependencies_ready())
            return false;

        this->m_actor.init(
            *this->m_desc_allocator,
            *this->m_desc_layout,
            this->m_phys_device,
            this->m_logi_device
        );

        return true;
    }

    void ActorSkinnedProxy::destroy() {
        this->m_actor.destroy(*this->m_desc_allocator, this->m_logi_device);
    }

    bool ActorSkinnedProxy::is_ready() const {
        return this->m_actor.is_ready();
    }

    void ActorSkinnedProxy::notify_transform_change() {
        this->m_actor.notify_transform_change();
    }

}


// MeshVK
namespace dal {

    void MeshVK::init(
        const std::vector<VertexStatic>& vertices,
        const std::vector<uint32_t>& indices,
        dal::CommandPool& cmd_pool,
        const VkPhysicalDevice phys_device,
        const dal::LogicalDevice& logi_device
    ) {
        this->m_vertices.init_static(
            vertices,
            indices,
            cmd_pool,
            logi_device.queue_graphics(),
            phys_device,
            logi_device.get()
        );
    }

    void MeshVK::destroy(const VkDevice logi_device) {
        this->m_vertices.destroy(logi_device);
    }

}


// MeshProxy
namespace dal {

    void MeshProxy::give_dependencies(
        dal::CommandPool& cmd_pool,
        const VkPhysicalDevice phys_device,
        const dal::LogicalDevice& logi_device
    ) {
        this->m_cmd_pool    = &cmd_pool;
        this->m_phys_device = phys_device;
        this->m_logi_device = &logi_device;
    }

    void MeshProxy::clear_dependencies() {
        this->m_cmd_pool    = nullptr;
        this->m_phys_device = VK_NULL_HANDLE;
        this->m_logi_device = nullptr;
    }

    bool MeshProxy::are_dependencies_ready() const {
        return (
            this->m_cmd_pool    != nullptr        &&
            this->m_phys_device != VK_NULL_HANDLE &&
            this->m_logi_device != nullptr
        );
    }

    // Overridings

    bool MeshProxy::init_mesh(const std::vector<VertexStatic>& vertices, const std::vector<uint32_t>& indices) {
        if (!this->are_dependencies_ready())
            return false;

        this->m_mesh.init(vertices, indices, *this->m_cmd_pool, this->m_phys_device, *this->m_logi_device);
        return true;
    }

    void MeshProxy::destroy() {
        this->m_mesh.destroy(this->m_logi_device->get());
    }

    bool MeshProxy::is_ready() const {
        return this->m_mesh.is_ready();
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
        const SamplerTexture& sampler,
        const dal::DescLayout_PerMaterial& layout_per_material,
        const VkDevice logi_device
    ) {
        if (this->is_ready())
            return true;
        if (!this->m_material.m_albedo_map->is_ready())
            return false;

        this->m_material.m_descset = desc_pool.allocate(layout_per_material, logi_device);

        auto& albedo_map = dal::handle_cast(this->m_material.m_albedo_map);

        this->m_material.m_descset.record_material(
            this->m_material.m_ubuf,
            albedo_map.raw_view(),
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
        const dal::ModelStatic& model_data,
        dal::CommandPool& cmd_pool,
        ITextureManager& tex_man,
        const char* const fallback_file_namespace,
        const dal::DescLayout_PerActor& layout_per_actor,
        const dal::DescLayout_PerMaterial& layout_per_material,
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

    void ModelRenderer::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_units)
            x.destroy(logi_device);
        this->m_units.clear();

        for (auto& x : this->m_units_alpha)
            x.destroy(logi_device);
        this->m_units_alpha.clear();

        this->m_desc_pool.destroy(logi_device);
    }

    bool ModelRenderer::fetch_one_resource(const dal::DescLayout_PerMaterial& layout_per_material, const SamplerTexture& sampler, const VkDevice logi_device) {
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


// ModelProxy
namespace dal {

    ModelProxy::~ModelProxy() {
        this->destroy();
        this->clear_dependencies();
    }

    void ModelProxy::give_dependencies(
        CommandPool&                  cmd_pool,
        ITextureManager&              tex_man,
        DescLayout_PerActor const&    layout_per_actor,
        DescLayout_PerMaterial const& layout_per_material,
        SamplerTexture const&         sampler,
        VkQueue                       graphics_queue,
        VkPhysicalDevice              phys_device,
        VkDevice                      logi_device
    ) {
        m_cmd_pool = &cmd_pool;
        m_tex_man = &tex_man;
        m_layout_per_actor = &layout_per_actor;
        m_layout_per_material = &layout_per_material;
        m_sampler = &sampler;
        m_graphics_queue = graphics_queue;
        m_phys_device = phys_device;
        m_logi_device = logi_device;
    }

    void ModelProxy::clear_dependencies() {
        m_cmd_pool = nullptr;
        m_tex_man = nullptr;
        m_layout_per_actor = nullptr;
        m_layout_per_material = nullptr;
        m_sampler = nullptr;
        m_graphics_queue = VK_NULL_HANDLE;
        m_phys_device = VK_NULL_HANDLE;
        m_logi_device = VK_NULL_HANDLE;
    }

    bool ModelProxy::are_dependencies_ready() const {
        return (
            m_cmd_pool != nullptr &&
            m_tex_man != nullptr &&
            m_layout_per_actor != nullptr &&
            m_layout_per_material != nullptr &&
            m_sampler != nullptr &&
            m_graphics_queue != VK_NULL_HANDLE &&
            m_phys_device != VK_NULL_HANDLE &&
            m_logi_device != VK_NULL_HANDLE
        );
    }

    const char* ModelProxy::name() const {
        return this->m_name.c_str();
    }

    bool ModelProxy::init_model(const char* const name, const dal::ModelStatic& model_data, const char* const fallback_namespace) {
        if (!this->are_dependencies_ready())
            return false;

        this->m_name = name;

        this->m_model.init(
            model_data,
            *this->m_cmd_pool,
            *this->m_tex_man,
            fallback_namespace,
            *this->m_layout_per_actor,
            *this->m_layout_per_material,
            this->m_graphics_queue,
            this->m_phys_device,
            this->m_logi_device
        );

        return true;
    }

    bool ModelProxy::prepare() {
        return this->m_model.fetch_one_resource(
            *this->m_layout_per_material,
            *this->m_sampler,
            this->m_logi_device
        );
    }

    void ModelProxy::destroy() {
        this->m_model.destroy(this->m_logi_device);
    }

    bool ModelProxy::is_ready() const {
        return this->m_model.is_ready();
    }

}


// ModelRenderer
namespace dal {

    void ModelSkinnedRenderer::upload_meshes(
        const dal::ModelSkinned& model_data,
        dal::CommandPool& cmd_pool,
        ITextureManager& tex_man,
        const char* const fallback_file_namespace,
        const dal::DescLayout_PerActor& layout_per_actor,
        const dal::DescLayout_PerMaterial& layout_per_material,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

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

    void ModelSkinnedRenderer::destroy(const VkDevice logi_device) {
        for (auto& x : this->m_units)
            x.destroy(logi_device);
        this->m_units.clear();

        for (auto& x : this->m_units_alpha)
            x.destroy(logi_device);
        this->m_units_alpha.clear();

        this->m_desc_pool.destroy(logi_device);
    }

    bool ModelSkinnedRenderer::fetch_one_resource(const dal::DescLayout_PerMaterial& layout_per_material, const SamplerTexture& sampler, const VkDevice logi_device) {
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


// ModelSkinnedProxy
namespace dal {

    ModelSkinnedProxy::ModelSkinnedProxy() {
        this->clear_dependencies();
    }

    ModelSkinnedProxy::~ModelSkinnedProxy() {
        this->destroy();
        this->clear_dependencies();
    }

    void ModelSkinnedProxy::give_dependencies(
        CommandPool&                  cmd_pool,
        ITextureManager&              tex_man,
        DescLayout_PerActor const&    layout_per_actor,
        DescLayout_PerMaterial const& layout_per_material,
        SamplerTexture const&         sampler,
        VkQueue                       graphics_queue,
        VkPhysicalDevice              phys_device,
        VkDevice                      logi_device
    ) {
        m_cmd_pool = &cmd_pool;
        m_tex_man = &tex_man;
        m_layout_per_actor = &layout_per_actor;
        m_layout_per_material = &layout_per_material;
        m_sampler = &sampler;
        m_graphics_queue = graphics_queue;
        m_phys_device = phys_device;
        m_logi_device = logi_device;
    }

    void ModelSkinnedProxy::clear_dependencies() {
        m_cmd_pool = nullptr;
        m_tex_man = nullptr;
        m_layout_per_actor = nullptr;
        m_layout_per_material = nullptr;
        m_sampler = nullptr;
        m_graphics_queue = VK_NULL_HANDLE;
        m_phys_device = VK_NULL_HANDLE;
        m_logi_device = VK_NULL_HANDLE;
    }

    bool ModelSkinnedProxy::are_dependencies_ready() const {
        return (
            m_cmd_pool != nullptr &&
            m_tex_man != nullptr &&
            m_layout_per_actor != nullptr &&
            m_layout_per_material != nullptr &&
            m_sampler != nullptr &&
            m_graphics_queue != VK_NULL_HANDLE &&
            m_phys_device != VK_NULL_HANDLE &&
            m_logi_device != VK_NULL_HANDLE
        );
    }

    // Overridings

    const char* ModelSkinnedProxy::name() const {
        return this->m_name.c_str();
    }

    bool ModelSkinnedProxy::init_model(const char* const name, const dal::ModelSkinned& model_data, const char* const fallback_namespace) {
        if (!this->are_dependencies_ready())
            return false;

        this->m_name = name;

        this->m_model.upload_meshes(
            model_data,
            *this->m_cmd_pool,
            *this->m_tex_man,
            fallback_namespace,
            *this->m_layout_per_actor,
            *this->m_layout_per_material,
            this->m_graphics_queue,
            this->m_phys_device,
            this->m_logi_device
        );

        return true;
    }

    bool ModelSkinnedProxy::prepare() {
        return this->m_model.fetch_one_resource(
            *this->m_layout_per_material,
            *this->m_sampler,
            this->m_logi_device
        );
    }

    void ModelSkinnedProxy::destroy() {
        this->m_model.destroy(this->m_logi_device);
    }

}
