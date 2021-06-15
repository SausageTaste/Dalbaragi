#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "d_actor.h"
#include "d_renderer.h"


namespace dal {

    namespace cpnt {

        struct Model {
            HRenModel m_model;
        };

        struct Actor {
            std::vector<HActor> m_actors;
        };

    }


    class Scene {

    public:
        entt::registry m_registry;

        EulerCamera m_euler_camera;

    public:
        Scene();

        RenderList make_render_list();

    };

}
