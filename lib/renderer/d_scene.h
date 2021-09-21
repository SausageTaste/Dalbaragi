#pragma once

#include <entt/entt.hpp>

#include "d_geometry.h"
#include "d_render_cpnt.h"


namespace dal::cpnt {

    struct ActorStatic {
        HRenModel m_model;
        HActor m_actor;
    };

    struct ActorAnimated {
        HRenModelSkinned m_model;
        HActorSkinned m_actor;
    };

}


namespace dal::scene {

    struct PortalPlane {
        std::array<glm::vec3, 4> m_vertices;
        dal::PlaneOriented m_plane;
    };

    struct PortalPair {
        std::array<PortalPlane, 2> m_portals;
    };

}


namespace dal {


    class Scene {

    public:
        entt::registry m_registry;

        EulerCamera m_euler_camera;

        DLight m_sun_light, m_moon_light, m_selected_dlight;
        std::vector<SLight> m_slights;
        std::vector<PLight> m_plights;
        glm::vec3 m_ambient_light;

        scene::PortalPair m_portal;

    public:
        Scene();

        void update();

    };

}
