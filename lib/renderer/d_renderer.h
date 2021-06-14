#pragma once

#include <memory>
#include <vector>

#include "d_actor.h"
#include "d_filesystem.h"
#include "d_model_data.h"
#include "d_image_parser.h"


namespace dal {

    class ITexture {

    public:
        virtual ~ITexture() = default;

        virtual void destroy() = 0;

        virtual bool is_ready() const = 0;

    };


    class IRenModel {

    public:
        virtual ~IRenModel() = default;

        virtual void destroy() = 0;

        virtual bool is_ready() const = 0;

    };


    class IActor {

    public:
        Transform m_transform;

    public:
        virtual ~IActor() = default;

        virtual void destroy() = 0;

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


    class ITextureManager {

    public:
        virtual ~ITextureManager() = default;

        virtual HTexture request_texture(const dal::ResPath& respath) = 0;

    };


    class IRenderer {

    public:
        virtual ~IRenderer() = default;

        virtual void update(const ICamera& camera, const RenderList& render_list) = 0;

        virtual void wait_idle() {}

        virtual void on_screen_resize(const unsigned width, const unsigned height) {}

        virtual HTexture create_texture() { return nullptr; }

        virtual HRenModel create_model() { return nullptr; }

        virtual HActor create_actor() { return nullptr; }

        virtual bool init_texture(ITexture& tex, const ImageData& img_data) { return false; }

        virtual bool init_model(IRenModel& model, const dal::ModelStatic& model_data, const char* const fallback_namespace) { return false; }

        virtual bool prepare(IRenModel& model) { return false; }

    };

}
