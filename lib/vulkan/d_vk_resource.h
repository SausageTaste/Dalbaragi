#pragma once

#include <vector>

#include "d_image_obj.h"
#include "d_model_renderer.h"


namespace dal {

    class VulkanResourceManager {

    private:
        std::vector< std::shared_ptr<TextureUnit>          > m_textures;
        std::vector< std::shared_ptr<ModelRenderer>        > m_models;
        std::vector< std::shared_ptr<ModelSkinnedRenderer> > m_skinned_models;
        std::vector< std::shared_ptr<ActorVK>              > m_actors;
        std::vector< std::shared_ptr<ActorSkinnedVK>       > m_skinned_actors;

    public:
        ~VulkanResourceManager();

        void destroy();

        HTexture create_texture();

        HRenModel create_model();

        HRenModelSkinned create_model_skinned();

        HActor create_actor();

        HActorSkinned create_actor_skinned();

    };

}
