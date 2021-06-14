#pragma once

#include <memory>
#include <vector>

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


    class IActor {

    public:
        Transform m_transform;

    public:
        virtual ~IActor() = default;

        virtual void apply_changes() = 0;

    };


    using HTexture  = std::shared_ptr<ITexture>;
    using HRenModel = std::shared_ptr<IRenModel>;
    using HActor    = std::shared_ptr<IActor>;


    struct RenderPair {
        std::vector<HActor> m_actors;
        HRenModel m_model;
    };

    using RenderList = std::vector<RenderPair>;


    class IRenderer {

    public:
        virtual ~IRenderer() = default;

        virtual void update(const ICamera& camera, const RenderList& render_list) = 0;

        virtual void wait_idle() = 0;

        virtual void on_screen_resize(const unsigned width, const unsigned height) = 0;

    };

}
