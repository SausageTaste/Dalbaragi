#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "d_actor.h"
#include "d_renderer.h"
#include "d_animation.h"


namespace dal {

    namespace cpnt {

        struct Model {
            HRenModel m_model;
        };

        struct ModelSkinned {
            HRenModelSkinned m_model;
        };

        struct Actor {
            std::vector<HActor> m_actors;
        };

        struct ActorAnimated {
            std::vector<HActorSkinned> m_actors;
        };

    }


    class Scene {

    public:
        entt::registry m_registry;

        EulerCamera m_euler_camera;

    public:
        Scene();

        void update();

        RenderList make_render_list();

    };

}
