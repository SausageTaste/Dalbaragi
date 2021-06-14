#include "d_scene.h"


namespace dal {

    Scene::Scene() {
        this->m_euler_camera.m_pos = { 2.68, 1.91, 0 };
        this->m_euler_camera.m_rotations = { -0.22, glm::radians<float>(90), 0 };
    }

    RenderList Scene::make_render_list() {
        RenderList output;

        auto view = this->m_registry.view<cpnt::Model, cpnt::Actor>();
        view.each([&output](cpnt::Model& model, cpnt::Actor& actor) {
            auto& one = output.emplace_back();

            one.m_model = model.m_model;
            one.m_actors = actor.m_actors;
        });

        return output;
    }

}
