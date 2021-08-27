#include "d_renderer.h"


namespace dal {

    void RenderList::add_actor(const HActor& actor, const HRenModel& model) {
        for (auto& x : this->m_static_models) {
            if (x.m_model == model) {
                x.m_actors.push_back(actor);
                return;
            }
        }

        auto& one = this->m_static_models.emplace_back();
        one.m_model = model;
        one.m_actors.push_back(actor);
    }

    void RenderList::add_actor(const HActorSkinned& actor, const HRenModelSkinned& model) {
        for (auto& x : this->m_skinned_models) {
            if (x.m_model == model) {
                x.m_actors.push_back(actor);
                return;
            }
        }

        auto& one = this->m_skinned_models.emplace_back();
        one.m_model = model;
        one.m_actors.push_back(actor);
    }

}
