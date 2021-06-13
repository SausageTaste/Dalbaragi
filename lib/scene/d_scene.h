#pragma once

#include <entt/entt.hpp>

#include "d_actor.h"


namespace dal {

    class Scene {

    public:
        entt::registry m_registry;

        EulerCamera m_euler_camera;

    public:
        Scene();

    };

}
