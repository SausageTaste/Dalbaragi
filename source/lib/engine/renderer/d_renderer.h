#pragma once

#include <functional>

#include "dal/util/indices.h"
#include "dal/util/filesystem.h"
#include "dal/util/image_parser.h"
#include "dal/util/model_data.h"
#include "d_scene.h"
#include "d_render_config.h"


namespace dal {


    class ITextureManager {

    public:
        virtual ~ITextureManager() = default;

        virtual HTexture request_texture(const dal::ResPath& respath) = 0;

    };


    class IRenderer {

    public:
        virtual ~IRenderer() = default;

        virtual void update(const ICamera& camera, dal::Scene& scene) {}

        virtual const FrameInFlightIndex& in_flight_index() const = 0;

        virtual void wait_idle() {}

        virtual void on_screen_resize(const unsigned width, const unsigned height) {}

        virtual void apply_config(const RendererConfig& config) {};

        virtual HTexture create_texture() { return nullptr; }

        virtual HMesh create_mesh() { return nullptr; }

        virtual HRenModel create_model() { return nullptr; }

        virtual HRenModelSkinned create_model_skinned() { return nullptr; }

        virtual HActor create_actor() { return nullptr; }

        virtual HActorSkinned create_actor_skinned() { return nullptr; }

        virtual void register_handle(HTexture& handle) {}

        virtual void register_handle(HMesh& handle) {}

        virtual void register_handle(HRenModel& handle) {}

        virtual void register_handle(HRenModelSkinned& handle) {}

        virtual void register_handle(HActor& handle) {}

        virtual void register_handle(HActorSkinned& handle) {}

    };


    using surface_create_func_t = std::function<uint64_t(void*)>;

}
