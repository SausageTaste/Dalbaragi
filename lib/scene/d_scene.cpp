#include "d_scene.h"


namespace dal {

    Scene::Scene() {
        this->m_euler_camera.m_pos = { 2.68, 1.91, 0 };
        this->m_euler_camera.m_rotations = { -0.22, glm::radians<float>(90), 0 };
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

        const auto t = dal::get_cur_sec();

        {
            output.m_dlights.push_back(this->m_dlight);
            output.m_dlights.back().m_pos = this->m_euler_camera.m_pos;
        }

        {
            auto& light = output.m_plights.emplace_back();
            light.m_pos = glm::vec3{sin(t) * 3 - 10, 1, cos(t) * 2};
            light.m_color = glm::vec3{0.5};
        }

        {
            auto& light = output.m_slights.emplace_back();
            light.set_direc_to_light(sin(t*0.7), 1, cos(t*0.7));
            light.m_pos = glm::vec3{6, 2, 0};
            light.m_color = glm::vec3{3};
            light.set_fade_start_degree(0);
            light.set_fade_end_degree(35);
        }

        {
            auto& light = output.m_slights.emplace_back();
            light.set_direc_to_light(0, 0, 1);
            light.m_pos = glm::vec3{cos(t*0.3) * 3, 1, 5};
            light.m_color = glm::vec3{3};
            light.set_fade_start_degree(0);
            light.set_fade_end_degree(35);
        }

        output.m_ambient_color = glm::vec3{0.01};

        return output;
    }

}
