#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "d_actor.h"
#include "d_renderer.h"
#include "d_animation.h"


namespace dal {

    namespace cpnt {

        struct ActorStatic {
            HRenModel m_model;
            HActor m_actor;
        };

        struct ActorAnimated {
            HRenModelSkinned m_model;
            HActorSkinned m_actor;
        };

    }


    class Scene {

    public:
        entt::registry m_registry;

        EulerCamera m_euler_camera;
        DLight m_dlight;

    public:
        Scene();

        void update();

        RenderList make_render_list();

    };

}
