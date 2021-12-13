#pragma once

#include <vector>

#include "d_image_obj.h"
#include "d_model_renderer.h"


namespace dal {

    class VulkanResourceManager {

    private:
        std::vector< std::shared_ptr<TextureProxy>         > m_textures;
        std::vector< std::shared_ptr<ModelRenderer>        > m_models;
        std::vector< std::shared_ptr<ModelSkinnedRenderer> > m_skinned_models;
        std::vector< std::shared_ptr<ActorProxy>           > m_actors;
        std::vector< std::shared_ptr<ActorSkinnedVK>       > m_skinned_actors;

    public:
        ~VulkanResourceManager();

        void destroy();

        HTexture create_texture(
            dal::CommandPool& cmd_pool,
            const VkQueue graphics_queue,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        HRenModel create_model();

        HRenModelSkinned create_model_skinned();

        HActor create_actor(
            DescAllocator& desc_allocator,
            const DescLayout_PerActor& desc_layout,
            VkPhysicalDevice phys_device,
            VkDevice logi_device
        );

        HActorSkinned create_actor_skinned();

    };

}
