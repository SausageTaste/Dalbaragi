#pragma once

#include <entt/entt.hpp>

#include "d_collider.h"
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
        HMesh m_mesh;
        HActor m_actor;
        PlaneOriented m_plane;
        TriangleSoup m_collider;

        std::optional<SegmentIntersectionInfo> find_intersection(const Segment& seg) const;

        PlaneOriented make_transformed_plane() const;
    };

    struct PortalPair {
        std::array<PortalPlane, 2> m_portals;

        std::optional<glm::mat4> calc_mat_to_teleport(const dal::Segment& delta_cam_pos) const;
    };

    struct MirrorPlane {
        HMesh m_mesh;
        HActor m_actor;
        dal::Plane m_plane;
    };

    struct HorizontalWater {
        HMesh m_mesh;
        dal::Plane m_plane;
    };

}


namespace dal {

    class Scene {

    private:
        using camera_t = dal::EulerCamera;

    public:
        entt::registry m_registry;

        camera_t m_euler_camera;

        DLight m_sun_light, m_moon_light, m_selected_dlight;
        std::vector<SLight> m_slights;
        std::vector<PLight> m_plights;
        glm::vec3 m_ambient_light;

        std::vector<scene::MirrorPlane> m_mirrors;
        std::vector<scene::PortalPair> m_portal_pairs;
        std::vector<scene::HorizontalWater> m_water_planes;

    private:
        camera_t m_prev_camera;

    public:
        Scene();

        void update();

    };

}
