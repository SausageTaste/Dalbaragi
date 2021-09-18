#include "d_scene.h"


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
        {
            auto view = this->m_registry.view<cpnt::ActorAnimated>();
            view.each([](cpnt::ActorAnimated& actor) {
                update_anime_state(actor.m_actor->m_anim_state, actor.m_model->animations(), actor.m_model->skeleton());
            });
        }
    }

    RenderList Scene::make_render_list() {
        RenderList output;

        const auto t = dal::get_cur_sec();

        {
            auto view = this->m_registry.view<cpnt::ActorStatic>();
            view.each([&output](cpnt::ActorStatic& actor) {
                output.add_actor(actor.m_actor, actor.m_model);
            });
        }

        {
            auto view = this->m_registry.view<cpnt::ActorAnimated>();
            view.each([&output](cpnt::ActorAnimated& actor) {
                output.add_actor(actor.m_actor, actor.m_model);
            });
        }

        {
            output.m_mirror_vertices[0] = glm::vec3{-1, 1, 0};
            output.m_mirror_vertices[1] = glm::vec3{-1, -1, 0};
            output.m_mirror_vertices[2] = glm::vec3{1, -1, 0};
            output.m_mirror_vertices[3] = glm::vec3{1, 1, 0};

            const auto x = dal::get_cur_sec() * 20;
            const auto translate = glm::translate(glm::mat4{1}, glm::vec3{3, 1, 0});
            const auto rotation = glm::rotate(glm::mat4{1}, glm::radians<float>(x), glm::vec3{1, 2, 3});
            const auto m = translate * rotation;

            for (int i = 0; i < 4; ++i) {
                output.m_mirror_vertices[i] = m * glm::vec4{output.m_mirror_vertices[i], 1};
            }
        }

        {
            const auto sun_direc = glm::normalize(glm::vec3{ 10.0 * cos(t * 0.1), 10.0 * sin(t * 0.1), 2 });
            const auto is_sun = glm::dot(glm::vec3(0, 1, 0), sun_direc) >= -0.1;

            if (is_sun) {
                output.m_dlights.push_back(this->m_sun_light);
                output.m_dlights.back().set_direc_to_light(sun_direc);
            }
            else {
                output.m_dlights.push_back(this->m_moon_light);
                output.m_dlights.back().set_direc_to_light(glm::vec3{0, 2, 0} - sun_direc);
            }
            output.m_dlights.back().m_pos = this->m_euler_camera.m_pos;
        }

        output.m_plights = this->m_plights;
        output.m_slights = this->m_slights;
        output.m_ambient_color = this->m_ambient_light;

        return output;
    }

}
