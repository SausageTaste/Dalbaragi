#include "d_scene.h"

#include <fmt/format.h>

#include "d_logger.h"


namespace {

    const std::array<glm::vec4, 4> TEMPLATE_VERTICES{
        glm::vec4{-1,  1, 0, 1},
        glm::vec4{-1, -1, 0, 1},
        glm::vec4{ 1, -1, 0, 1},
        glm::vec4{ 1,  1, 0, 1},
    };


    auto make_plane_of_quad(const std::array<glm::vec3, 4>& vertices) {
        const auto center  = (vertices[0] + vertices[2]) * 0.5f;
        const auto top     = (vertices[0] + vertices[3]) * 0.5f;
        const auto winding = vertices[0];

        return dal::PlaneOriented{
            center,
            top,
            winding
        };
    };

}


namespace dal::scene {

    std::optional<SegmentIntersectionInfo> PortalPlane::find_intersection(const Segment& seg) const {
        const auto tri0 = dal::Triangle{ this->m_vertices[0], this->m_vertices[1], this->m_vertices[2] };
        const auto tri1 = dal::Triangle{ this->m_vertices[0], this->m_vertices[2], this->m_vertices[3] };

        {
            const auto result = tri0.find_intersection(seg, true);
            if (result)
                return result;
        }

        {
            const auto result = tri1.find_intersection(seg, true);
            if (result)
                return result;
        }

        return std::nullopt;
    }

    std::optional<glm::mat4> PortalPair::calc_mat_to_teleport(const dal::Segment& seg) const {
        if (seg.length_sqr() != 0.f) {
            const auto intersection = this->m_portals[0].find_intersection(seg);
            if (intersection) {
                const auto a = intersection->calc_intersecting_point(seg);
                const auto portal_mat = dal::make_portal_mat(this->m_portals[1].m_plane, this->m_portals[0].m_plane);
                return portal_mat;
            }
        }

        if (seg.length_sqr() != 0.f) {
            const auto intersection = this->m_portals[1].find_intersection(seg);
            if (intersection) {
                const auto a = intersection->calc_intersecting_point(seg);
                const auto portal_mat = dal::make_portal_mat(this->m_portals[0].m_plane, this->m_portals[1].m_plane);
                return portal_mat;
            }
        }

        return std::nullopt;
    }

}


namespace dal {

    Scene::Scene() {
        this->m_euler_camera.pos() = { 2.68, 1.91, 0 };
        this->m_euler_camera.set_rot_xyz(-0.22, glm::radians<float>(90), 0);
        this->m_prev_camera = this->m_euler_camera;

        {
            this->m_sun_light.m_atmos_intensity = 40;
            this->m_sun_light.m_color = glm::vec3{2};

            this->m_moon_light.m_atmos_intensity = 0.05;
            this->m_moon_light.m_color = glm::vec3{0.05, 0.05, 0.15};
        }

        // Mirrors
        {
            auto& mirror = this->m_mirrors.emplace_back();

            const auto rotation = glm::rotate(glm::mat4{1}, glm::radians<float>(90), glm::vec3{0, 1, 0});
            const auto translate = glm::translate(glm::mat4{1}, glm::vec3{-14, 1.2, 0});
            const auto mat = translate * rotation;

            for (size_t i = 0; i < 4; ++i) {
                mirror.m_vertices[i] = mat * ::TEMPLATE_VERTICES[i];
            }

            mirror.m_plane = dal::Plane{mirror.m_vertices[0], mirror.m_vertices[1], mirror.m_vertices[2]};
        }

        // Portal pair
        /*{
            this->m_portal_pairs.emplace_back();
            this->m_portal_pairs.emplace_back();
        }*/

        // Water
        /*{
            auto& water = this->m_water_planes.emplace_back();

            water.m_height = 0.2;
            water.m_plane = dal::Plane{ glm::vec3{0, water.m_height, 0}, glm::vec3{0, 1, 0} };
        }*/
    }

    void Scene::update() {
        const auto t = dal::get_cur_sec();

        // Update animations
        {
            auto view = this->m_registry.view<cpnt::ActorAnimated>();
            view.each([](cpnt::ActorAnimated& actor) {
                update_anime_state(actor.m_actor->m_anim_state, actor.m_model->animations(), actor.m_model->skeleton());
            });
        }

        // Apply portal teleportation
        {
            const Segment seg{
                this->m_prev_camera.pos(),
                this->m_euler_camera.pos() - this->m_prev_camera.pos()
            };

            for (auto& ppair : this->m_portal_pairs) {
                const auto portal_mat = ppair.calc_mat_to_teleport(seg);
                if (portal_mat) {
                    this->m_euler_camera = this->m_euler_camera.transform(*portal_mat);
                    break;  // It doesn't make sense to teleport though multiple portal pairs
                }
            }
        }

        // Directional light
        {
            const auto sun_direc = glm::normalize(glm::vec3{ 10.0 * cos(t * 0.1), 10.0 * sin(t * 0.1), 2 });
            const auto is_sun = glm::dot(glm::vec3(0, 1, 0), sun_direc) >= -0.1;

            if (is_sun) {
                this->m_selected_dlight = this->m_sun_light;
                this->m_selected_dlight.set_direc_to_light(sun_direc);
            }
            else {
                this->m_selected_dlight = this->m_moon_light;
                this->m_selected_dlight.set_direc_to_light(glm::vec3{0, 2, 0} - sun_direc);
            }
            this->m_selected_dlight.m_pos = this->m_euler_camera.pos();
        }

        // Move portals
        for (size_t i = 0; i < this->m_portal_pairs.size(); ++i) {
            auto& ppair = this->m_portal_pairs[i];

            const auto translate1 = glm::translate(glm::mat4{1}, glm::vec3{-sin(t * 0.3) * 3 - 4.5, 1 + 2*i, 1});
            const auto translate2 = glm::translate(glm::mat4{1}, glm::vec3{3, 1, -static_cast<int>(2 + 2*i)});
            const auto rotation1 = glm::rotate(glm::mat4{1}, glm::radians<float>(t * 20), glm::vec3{0, 1, 0}) * glm::rotate(glm::mat4{1}, glm::radians<float>(-90), glm::vec3{1, 0, 0});
            const auto rotation2 = glm::rotate(glm::mat4{1}, glm::radians<float>(cos(t) * 20), glm::vec3{0, 1, 0});

            const auto m1 = translate1 * rotation1;
            const auto m2 = translate2 * rotation2;

            for (int i = 0; i < 4; ++i) {
                ppair.m_portals[0].m_vertices[i] = m1 * TEMPLATE_VERTICES[i];
                ppair.m_portals[1].m_vertices[i] = m2 * TEMPLATE_VERTICES[i];
            }

            ppair.m_portals[0].m_plane = ::make_plane_of_quad(ppair.m_portals[0].m_vertices);
            ppair.m_portals[1].m_plane = ::make_plane_of_quad(ppair.m_portals[1].m_vertices);
        }

        // Update member variables
        {
            this->m_prev_camera = this->m_euler_camera;
        }
    }

}
