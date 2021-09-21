#pragma once

#include <entt/entt.hpp>

#include "d_render_cpnt.h"


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

        DLight m_sun_light, m_moon_light;
        std::vector<SLight> m_slights;
        std::vector<PLight> m_plights;
        glm::vec3 m_ambient_light;

    public:
        Scene();

        void update();

        RenderList make_render_list();

    };

}
