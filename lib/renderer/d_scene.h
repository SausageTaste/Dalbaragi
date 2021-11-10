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

        std::optional<SegmentIntersectionInfo> find_intersection(const Segment& seg) const;
    };

    struct PortalPair {
        std::array<PortalPlane, 2> m_portals;
    };

    struct MirrorPlane {
        std::array<glm::vec3, 4> m_vertices;
        dal::Plane m_plane;
    };

    struct HorizontalWater {
        dal::Plane m_plane;
        float m_height;
    };

}


namespace dal {

    class Scene {

    public:
        entt::registry m_registry;

        QuatCamera m_euler_camera;

        DLight m_sun_light, m_moon_light, m_selected_dlight;
        std::vector<SLight> m_slights;
        std::vector<PLight> m_plights;
        glm::vec3 m_ambient_light;

        std::vector<scene::MirrorPlane> m_mirrors;
        scene::PortalPair m_portal;
        scene::HorizontalWater m_hor_water;

    private:
        QuatCamera m_prev_camera;

    public:
        Scene();

        void update();

    };

}
