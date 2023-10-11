#pragma once

#include "d_renderer.h"
#include "d_vert_data.h"
#include "d_uniform.h"
#include "d_image_obj.h"
#include "dal/util/filesystem.h"
#include "d_vk_device.h"


namespace dal {

    class ActorVK {

    private:
        std::vector<DescSet> m_desc;
        dal::UniformBufferArray<dal::U_PerActor> m_ubuf_per_actor;

    public:
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

        void apply_transform(const FrameInFlightIndex& index, const dal::Transform& transform, const VkDevice logi_device);

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

        void apply_transform(const FrameInFlightIndex& index);

        auto& desc_set_at(const FrameInFlightIndex& index) const {
            return this->get().desc_set_at(index);
        }

        // Overridings

        bool init() override;

        void destroy() override;

        bool is_ready() const override;

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


    class MeshVK {

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

        bool is_ready() const {
            return this->m_vertices.is_ready();
        }

        auto index_size() const {
            return this->m_vertices.index_size();
        }

        auto vertex_buffer() const {
            return this->m_vertices.vertex_buffer();
        }

        auto index_buffer() const {
            return this->m_vertices.index_buffer();
        }

    };


    class MeshProxy : public IMesh {

    private:
        MeshVK m_mesh;

        dal::CommandPool*         m_cmd_pool    = nullptr;
        VkPhysicalDevice          m_phys_device = VK_NULL_HANDLE;
        dal::LogicalDevice const* m_logi_device = nullptr;

    public:
        void give_dependencies(
            dal::CommandPool& cmd_pool,
            const VkPhysicalDevice phys_device,
            const dal::LogicalDevice& logi_device
        );

        void clear_dependencies();

        bool are_dependencies_ready() const;

        auto& get() {
            return this->m_mesh;
        }

        auto& get() const {
            return this->m_mesh;
        }

        // Overridings

        bool init_mesh(const std::vector<VertexStatic>& vertices, const std::vector<uint32_t>& indices) override;

        void destroy() override;

        bool is_ready() const override;

    };


    inline auto& handle_cast(dal::IMesh& model) {
        return dynamic_cast<dal::MeshProxy&>(model);
    }

    inline auto& handle_cast(const dal::IMesh& model) {
        return dynamic_cast<const dal::MeshProxy&>(model);
    }

    inline auto& handle_cast(dal::HMesh& model) {
        return dynamic_cast<dal::MeshProxy&>(*model);
    }

    inline auto& handle_cast(const dal::HMesh& model) {
        return dynamic_cast<const dal::MeshProxy&>(*model);
    }


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


    class ModelRenderer {

    private:
        std::vector<RenderUnit> m_units;
        std::vector<RenderUnit> m_units_alpha;
        DescPool m_desc_pool;

    public:
        ~ModelRenderer() = default;

        void init(
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

        void destroy(const VkDevice logi_device);

        bool fetch_one_resource(const DescLayout_PerMaterial& layout_per_material, const SamplerTexture& sampler, const VkDevice logi_device);

        bool is_ready() const;

        auto& render_units() const {
            return this->m_units;
        }

        auto& render_units_alpha() const {
            return this->m_units_alpha;
        }

    };


    class ModelProxy : public IRenModel {

    private:
        ModelRenderer m_model;
        std::string m_name;

        CommandPool*                  m_cmd_pool;
        ITextureManager*              m_tex_man;
        DescLayout_PerActor const*    m_layout_per_actor;
        DescLayout_PerMaterial const* m_layout_per_material;
        SamplerTexture const*         m_sampler;
        VkQueue                       m_graphics_queue;
        VkPhysicalDevice              m_phys_device;
        VkDevice                      m_logi_device;

    public:
        ModelProxy() {
            this->clear_dependencies();
        }

        ~ModelProxy();

        void give_dependencies(
            CommandPool&                  cmd_pool,
            ITextureManager&              tex_man,
            DescLayout_PerActor const&    layout_per_actor,
            DescLayout_PerMaterial const& layout_per_material,
            SamplerTexture const&         sampler,
            VkQueue                       graphics_queue,
            VkPhysicalDevice              phys_device,
            VkDevice                      logi_device
        );

        void clear_dependencies();

        bool are_dependencies_ready() const;

        auto& get() {
            return this->m_model;
        }

        auto& get() const {
            return this->m_model;
        }

        // Overridings

        const char* name() const override;

        bool init_model(const char* const name, const dal::ModelStatic& model_data, const char* const fallback_namespace) override;

        bool prepare() override;

        void destroy() override;

        bool is_ready() const override;

    };


    inline auto& handle_cast(dal::IRenModel& model) {
        return dynamic_cast<dal::ModelProxy&>(model);
    }

    inline auto& handle_cast(const dal::IRenModel& model) {
        return dynamic_cast<const dal::ModelProxy&>(model);
    }

    inline auto& handle_cast(dal::HRenModel& model) {
        return dynamic_cast<dal::ModelProxy&>(*model);
    }

    inline auto& handle_cast(const dal::HRenModel& model) {
        return dynamic_cast<const dal::ModelProxy&>(*model);
    }


    class ModelSkinnedRenderer {

    private:
        std::vector<RenderUnit> m_units;
        std::vector<RenderUnit> m_units_alpha;
        std::vector<Animation> m_animations;
        SkeletonInterface m_skeleton_interf;
        DescPool m_desc_pool;

    public:
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

        void destroy(const VkDevice logi_device);

        bool fetch_one_resource(const DescLayout_PerMaterial& layout_per_material, const SamplerTexture& sampler, const VkDevice logi_device);

        bool is_ready() const;

        std::vector<Animation>& animations() {
            return this->m_animations;
        }

        const std::vector<Animation>& animations() const {
            return this->m_animations;
        }

        const SkeletonInterface& skeleton() const {
            return this->m_skeleton_interf;
        }

        auto& render_units() const {
            return this->m_units;
        }

        auto& render_units_alpha() const {
            return this->m_units_alpha;
        }

    };


    class ModelSkinnedProxy : public IRenModelSkineed {

    private:
        ModelSkinnedRenderer m_model;
        std::string m_name;

        CommandPool*                  m_cmd_pool;
        ITextureManager*              m_tex_man;
        DescLayout_PerActor const*    m_layout_per_actor;
        DescLayout_PerMaterial const* m_layout_per_material;
        SamplerTexture const*         m_sampler;
        VkQueue                       m_graphics_queue;
        VkPhysicalDevice              m_phys_device;
        VkDevice                      m_logi_device;

    public:
        ModelSkinnedProxy();

        ~ModelSkinnedProxy() override;

        void give_dependencies(
            CommandPool&                  cmd_pool,
            ITextureManager&              tex_man,
            DescLayout_PerActor const&    layout_per_actor,
            DescLayout_PerMaterial const& layout_per_material,
            SamplerTexture const&         sampler,
            VkQueue                       graphics_queue,
            VkPhysicalDevice              phys_device,
            VkDevice                      logi_device
        );

        void clear_dependencies();

        bool are_dependencies_ready() const;

        auto& get() {
            return this->m_model;
        }

        auto& get() const {
            return this->m_model;
        }

        // Overridings

        const char* name() const override;

        bool init_model(const char* const name, const dal::ModelSkinned& model_data, const char* const fallback_namespace) override;

        bool prepare() override;

        void destroy() override;

        bool is_ready() const override {
            return this->m_model.is_ready();
        }

        std::vector<Animation>& animations() override {
            return this->m_model.animations();
        }

        const std::vector<Animation>& animations() const override {
            return this->m_model.animations();
        }

        const SkeletonInterface& skeleton() const override {
            return this->m_model.skeleton();
        }

    };


    inline auto& handle_cast(dal::IRenModelSkineed& model) {
        return dynamic_cast<dal::ModelSkinnedProxy&>(model);
    }

    inline auto& handle_cast(const dal::IRenModelSkineed& model) {
        return dynamic_cast<const dal::ModelSkinnedProxy&>(model);
    }

    inline auto& handle_cast(dal::HRenModelSkinned& model) {
        return dynamic_cast<dal::ModelSkinnedProxy&>(*model);
    }

    inline auto& handle_cast(const dal::HRenModelSkinned& model) {
        return dynamic_cast<const dal::ModelSkinnedProxy&>(*model);
    }

}
