#include "d_scene.h"


namespace dal {

    Scene::Scene() {
        this->m_euler_camera.m_pos = { 2.68, 1.91, 0 };
        this->m_euler_camera.m_rotations = { -0.22, glm::radians<float>(90), 0 };
    }

    void Scene::update() {
        {
            auto view = this->m_registry.view<cpnt::ModelSkinned, cpnt::ActorAnimated>();
            view.each([](cpnt::ModelSkinned& model, cpnt::ActorAnimated& actor) {
                for (auto& a : actor.m_actors)
                    update_anime_state(a->m_anim_state, model.m_model->animations(), model.m_model->skeleton());
            });
        }
    }

    RenderList Scene::make_render_list() {
        RenderList output;

        {
            auto view = this->m_registry.view<cpnt::Model, cpnt::Actor>();
            view.each([&output](cpnt::Model& model, cpnt::Actor& actor) {
                auto& one = output.m_static_models.emplace_back();

                one.m_model = model.m_model;
                one.m_actors = actor.m_actors;
            });
        }

        {
            auto view = this->m_registry.view<cpnt::ModelSkinned, cpnt::ActorAnimated>();
            view.each([&output](cpnt::ModelSkinned& model, cpnt::ActorAnimated& actor) {
                auto& one = output.m_skinned_models.emplace_back();

                one.m_model = model.m_model;
                one.m_actors = actor.m_actors;
            });
        }

        const auto t = dal::get_cur_sec();

        {
            auto& light = output.m_dlights.emplace_back();
            light.m_pos = this->m_euler_camera.m_pos;
            light.set_direc_to_light(1, 10, 1);
            light.m_color = glm::vec3{0.2, 0.2, 0.4};
        }

        {
            auto& light = output.m_dlights.emplace_back();
            light.m_pos = this->m_euler_camera.m_pos;
            light.set_direc_to_light(sin(t*0.5) * 1, cos(t*0.5) * 5, 1);
            light.m_color = glm::vec3{2};
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

        /*{
            auto& light = output.m_slights.emplace_back();
            light.set_direc_to_light(0, 0, -1);
            light.m_pos = glm::vec3{cos(t*0.3) * 10, 7, 0};
            light.m_color = glm::vec3{3};
            light.set_fade_start_degree(0);
            light.set_fade_end_degree(35);
        }*/

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
