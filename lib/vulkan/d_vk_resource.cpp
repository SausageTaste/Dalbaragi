#include "d_vk_resource.h"

#include "d_logger.h"


// VulkanResourceManager
namespace dal {

    VulkanResourceManager::~VulkanResourceManager() {
        dalAssert(this->m_textures.empty());
        dalAssert(this->m_models.empty());
        dalAssert(this->m_skinned_models.empty());
        dalAssert(this->m_actors.empty());
        dalAssert(this->m_skinned_actors.empty());
    }

    void VulkanResourceManager::destroy() {
        for (auto& x : this->m_textures) {
            x->destroy();
            x->clear_dependencies();
        }
        this->m_textures.clear();

        for (auto& x : this->m_models) {
            x->destroy();
        }
        this->m_models.clear();

        for (auto& x : this->m_skinned_models) {
            x->destroy();
        }
        this->m_skinned_models.clear();

        for (auto& x : this->m_actors) {
            x->destroy();
            x->clear_dependencies();
        }
        this->m_actors.clear();

        for (auto& x : this->m_skinned_actors) {
            x->destroy();
        }
        this->m_skinned_actors.clear();
    }

    HTexture VulkanResourceManager::create_texture(
        dal::CommandPool& cmd_pool,
        const VkQueue graphics_queue,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->m_textures.push_back(std::make_shared<TextureProxy>());
        auto& tex = this->m_textures.back();
        tex->give_dependencies(cmd_pool, graphics_queue, phys_device, logi_device);
        return tex;
    }

    HRenModel VulkanResourceManager::create_model() {
        this->m_models.push_back(std::make_shared<ModelRenderer>());
        return this->m_models.back();
    }

    HRenModelSkinned VulkanResourceManager::create_model_skinned() {
        this->m_skinned_models.push_back(std::make_shared<ModelSkinnedRenderer>());
        return this->m_skinned_models.back();
    }

    HActor VulkanResourceManager::create_actor(
        DescAllocator& desc_allocator,
        const DescLayout_PerActor& desc_layout,
        VkPhysicalDevice phys_device,
        VkDevice logi_device
    ) {
        this->m_actors.push_back(std::make_shared<ActorProxy>());
        auto& actor = this->m_actors.back();
        actor->give_dependencies(desc_allocator, desc_layout, phys_device, logi_device);
        return actor;
    }

    HActorSkinned VulkanResourceManager::create_actor_skinned() {
        this->m_skinned_actors.push_back(std::make_shared<ActorSkinnedVK>());
        return this->m_skinned_actors.back();
    }

}
