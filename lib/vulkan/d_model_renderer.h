#pragma once

#include "d_renderer.h"
#include "d_vert_data.h"
#include "d_uniform.h"
#include "d_image_obj.h"
#include "d_filesystem.h"
#include "d_vk_device.h"


namespace dal {

    class ActorVK {

    private:
        std::vector<DescSet> m_desc;
        dal::UniformBufferArray<dal::U_PerActor> m_ubuf_per_actor;

    public:
        Transform m_transform;
        size_t m_transform_update_needed = 0;

    public:
        void init(
            DescAllocator& desc_allocator,
            const DescLayout_PerActor& layout_per_actor,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(DescAllocator& desc_allocator, const VkDevice logi_device);

        bool is_ready() const;

        void notify_transform_change() {
            this->m_transform_update_needed = MAX_FRAMES_IN_FLIGHT;
        }

        void apply_transform(const FrameInFlightIndex& index, const VkDevice logi_device);

        auto& desc_set_at(const FrameInFlightIndex& index) const {
            return this->m_desc.at(index.get()).get();
        }

    };


    class ActorProxy : public IActor {

    private:
        ActorVK m_actor;

        DescAllocator*             m_desc_allocator = nullptr;
        DescLayout_PerActor const* m_desc_layout    = nullptr;
        VkPhysicalDevice           m_phys_device    = VK_NULL_HANDLE;
        VkDevice                   m_logi_device    = VK_NULL_HANDLE;

    public:
        ~ActorProxy();

        void give_dependencies(
            DescAllocator& desc_allocator,
            const DescLayout_PerActor& desc_layout,
            VkPhysicalDevice phys_device,
            VkDevice logi_device
        );

        void clear_dependencies();

        bool are_dependencies_ready() const;

        auto& get() {
            return this->m_actor;
        }

        auto& get() const {
            return this->m_actor;
        }

        // Overridings

        bool init() override;

        void destroy() override;

        bool is_ready() const override;

        dal::Transform& transform() override {
            return this->m_actor.m_transform;
        }

        const dal::Transform& transform() const override {
            return this->m_actor.m_transform;
        }

        void notify_transform_change() override;

    };


    inline auto& handle_cast(dal::IActor& actor) {
        return dynamic_cast<dal::ActorProxy&>(actor);
    }

    inline auto& handle_cast(const dal::IActor& actor) {
        return dynamic_cast<const dal::ActorProxy&>(actor);
    }

    inline auto& handle_cast(dal::HActor& actor) {
        return dynamic_cast<dal::ActorProxy&>(*actor.get());
    }

    inline auto& handle_cast(const dal::HActor& actor) {
        return dynamic_cast<const dal::ActorProxy&>(*actor.get());
    }


    class ActorSkinnedVK {

    private:
        std::vector<DescSet> m_desc;
        dal::UniformBufferArray<dal::U_PerActor> m_ubuf_per_actor;
        dal::UniformBufferArray<dal::U_AnimTransform> m_ubuf_anim;

    public:
        size_t m_transform_update_needed = 0;

    public:
        void init(
            DescAllocator& desc_allocator,
            const DescLayout_ActorAnimated& layout_actor,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(DescAllocator& desc_allocator, const VkDevice logi_device);

        bool is_ready() const;

        void notify_transform_change() {
            this->m_transform_update_needed = MAX_FRAMES_IN_FLIGHT;
        }

        void apply_transform(const FrameInFlightIndex& index, const dal::Transform& transform, const VkDevice logi_device);

        void apply_animation(const FrameInFlightIndex& index, const dal::AnimationState& anim_state, const VkDevice logi_device);

        auto& desc_set_at(const FrameInFlightIndex& index) const {
            return this->m_desc.at(index.get()).get();
        }

    };


    class ActorSkinnedProxy : public IActorSkinned {

    private:
        ActorSkinnedVK m_actor;

        DescAllocator*                  m_desc_allocator = nullptr;
        DescLayout_ActorAnimated const* m_desc_layout    = nullptr;
        VkPhysicalDevice                m_phys_device    = VK_NULL_HANDLE;
        VkDevice                        m_logi_device    = VK_NULL_HANDLE;

    public:
        ~ActorSkinnedProxy();

        void give_dependencies(
            DescAllocator& desc_allocator,
            const DescLayout_ActorAnimated& desc_layout,
            VkPhysicalDevice phys_device,
            VkDevice logi_device
        );

        void clear_dependencies();

        bool are_dependencies_ready() const;

        auto& get() {
            return this->m_actor;
        }

        auto& get() const {
            return this->m_actor;
        }

        auto& desc_set_at(const FrameInFlightIndex& index) const {
            return this->get().desc_set_at(index);
        }

        void apply_transform(const FrameInFlightIndex& index);

        void apply_animation(const FrameInFlightIndex& index);

        // Overridings

        bool init() override;

        void destroy() override;

        bool is_ready() const override;

        void notify_transform_change() override;

    };


    inline auto& handle_cast(dal::IActorSkinned& actor) {
        return dynamic_cast<dal::ActorSkinnedProxy&>(actor);
    }

    inline auto& handle_cast(const dal::IActorSkinned& actor) {
        return dynamic_cast<const dal::ActorSkinnedProxy&>(actor);
    }

    inline auto& handle_cast(dal::HActorSkinned& actor) {
        return dynamic_cast<dal::ActorSkinnedProxy&>(*actor.get());
    }

    inline auto& handle_cast(const dal::HActorSkinned& actor) {
        return dynamic_cast<const dal::ActorSkinnedProxy&>(*actor.get());
    }


    class MeshVK : public IMesh {

    private:
        VertexBuffer m_vertices;

    public:
        void init(
            const std::vector<VertexStatic>& vertices,
            const std::vector<uint32_t>& indices,
            dal::CommandPool& cmd_pool,
            const VkPhysicalDevice phys_device,
            const dal::LogicalDevice& logi_device
        );

        void destroy(const VkDevice logi_device);

        bool is_ready() const override {
            return this->m_vertices.is_ready();
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
        void init_static(
            const dal::RenderUnitStatic& unit_data,
            dal::CommandPool& cmd_pool,
            ITextureManager& tex_man,
            const char* const fallback_file_namespace,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void init_skinned(
            const dal::RenderUnitSkinned& unit_data,
            dal::CommandPool& cmd_pool,
            ITextureManager& tex_man,
            const char* const fallback_file_namespace,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        bool prepare(
            DescPool& desc_pool,
            const SamplerTexture& sampler,
            const DescLayout_PerMaterial& layout_per_material,
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
            ITextureManager& tex_man,
            const char* const fallback_file_namespace,
            const DescLayout_PerActor& layout_per_actor,
            const DescLayout_PerMaterial& layout_per_material,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy() override;

        bool fetch_one_resource(const DescLayout_PerMaterial& layout_per_material, const SamplerTexture& sampler, const VkDevice logi_device);

        bool is_ready() const override;

        auto& render_units() const {
            return this->m_units;
        }

        auto& render_units_alpha() const {
            return this->m_units_alpha;
        }

    };


    class ModelSkinnedRenderer : public IRenModelSkineed {

    private:
        std::vector<RenderUnit> m_units;
        std::vector<RenderUnit> m_units_alpha;
        std::vector<Animation> m_animations;
        SkeletonInterface m_skeleton_interf;
        DescPool m_desc_pool;

        VkDevice m_logi_device = VK_NULL_HANDLE;

    public:
        ~ModelSkinnedRenderer() override {
            this->destroy();
        }

        void init(
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void upload_meshes(
            const dal::ModelSkinned& model_data,
            dal::CommandPool& cmd_pool,
            ITextureManager& tex_man,
            const char* const fallback_file_namespace,
            const DescLayout_PerActor& layout_per_actor,
            const DescLayout_PerMaterial& layout_per_material,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy() override;

        bool fetch_one_resource(const DescLayout_PerMaterial& layout_per_material, const SamplerTexture& sampler, const VkDevice logi_device);

        bool is_ready() const override;

        std::vector<Animation>& animations() override {
            return this->m_animations;
        }

        const std::vector<Animation>& animations() const override {
            return this->m_animations;
        }

        const SkeletonInterface& skeleton() const override {
            return this->m_skeleton_interf;
        }

        auto& render_units() const {
            return this->m_units;
        }

        auto& render_units_alpha() const {
            return this->m_units_alpha;
        }

    };

}


namespace dal {

    inline auto& mesh_cast(IMesh& mesh) {
        return static_cast<MeshVK&>(mesh);
    }

    inline auto& mesh_cast(const IMesh& mesh) {
        return static_cast<const MeshVK&>(mesh);
    }

    inline auto& mesh_cast(HMesh& mesh) {
        return *static_cast<MeshVK*>(mesh.get());
    }

    inline auto& mesh_cast(const HMesh& mesh) {
        return *static_cast<const MeshVK*>(mesh.get());
    }

}
