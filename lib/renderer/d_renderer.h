#pragma once

#include <memory>

#include "d_actor.h"


namespace dal {

    class ITexture {

    public:
        virtual ~ITexture() = default;

        virtual bool is_ready() const = 0;

    };


    class IRenModel {

    public:
        virtual ~IRenModel() = default;

        virtual bool is_ready() const = 0;

    };


    class IRenderer {

    public:
        virtual ~IRenderer() = default;

        virtual void update(const ICamera& camera) = 0;

        virtual void wait_idle() = 0;

        virtual void on_screen_resize(const unsigned width, const unsigned height) = 0;

    };

}
