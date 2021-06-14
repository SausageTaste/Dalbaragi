#pragma once

#include "d_actor.h"


namespace dal {

    class IRenderer {

    public:
        virtual ~IRenderer() = default;

        virtual void update(const ICamera& camera) = 0;

        virtual void wait_idle() = 0;

        virtual void on_screen_resize(const unsigned width, const unsigned height) = 0;

    };

}
