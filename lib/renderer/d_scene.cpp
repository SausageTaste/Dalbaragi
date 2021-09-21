#include "d_scene.h"


namespace {

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


namespace dal {

    Scene::Scene() {
        this->m_euler_camera.m_pos = { 2.68, 1.91, 0 };
        this->m_euler_camera.m_rotations = { -0.22, glm::radians<float>(90), 0 };

        {
            this->m_sun_light.m_atmos_intensity = 40;
            this->m_sun_light.m_color = glm::vec3{2};

            this->m_moon_light.m_atmos_intensity = 0.05;
            this->m_moon_light.m_color = glm::vec3{0.05, 0.05, 0.15};
        }
    }

    void Scene::update() {
        const auto t = dal::get_cur_sec();

        {
            auto view = this->m_registry.view<cpnt::ActorAnimated>();
            view.each([](cpnt::ActorAnimated& actor) {
                update_anime_state(actor.m_actor->m_anim_state, actor.m_model->animations(), actor.m_model->skeleton());
            });
        }

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
            this->m_selected_dlight.m_pos = this->m_euler_camera.m_pos;
        }

        {
            const std::array<glm::vec4, 4> template_vertices{
                glm::vec4{-1,  1, 0, 1},
                glm::vec4{-1, -1, 0, 1},
                glm::vec4{ 1, -1, 0, 1},
                glm::vec4{ 1,  1, 0, 1},
            };

            const auto translate1 = glm::translate(glm::mat4{1}, glm::vec3{-sin(t * 0.3) * 3 - 4.5, 1, 1});
            const auto translate2 = glm::translate(glm::mat4{1}, glm::vec3{3, 1, -2});
            const auto rotation1 = glm::rotate(glm::mat4{1}, glm::radians<float>(180), glm::vec3{0, 1, 0});
            const auto rotation2 = glm::rotate(glm::mat4{1}, glm::radians<float>(0), glm::vec3{0, 1, 0});

            const auto m1 = translate1 * rotation1;
            const auto m2 = translate2 * rotation2;

            for (int i = 0; i < 4; ++i) {
                this->m_portal.m_portals[0].m_vertices[i] = m1 * template_vertices[i];
                this->m_portal.m_portals[1].m_vertices[i] = m2 * template_vertices[i];
            }

            this->m_portal.m_portals[0].m_plane = ::make_plane_of_quad(this->m_portal.m_portals[0].m_vertices);
            this->m_portal.m_portals[1].m_plane = ::make_plane_of_quad(this->m_portal.m_portals[1].m_vertices);
        }
    }

}
