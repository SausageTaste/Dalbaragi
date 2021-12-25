#pragma once

#include <functional>

#include "d_scene.h"
#include "d_indices.h"
#include "d_filesystem.h"
#include "d_model_data.h"
#include "d_image_parser.h"
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

    };


    using surface_create_func_t = std::function<uint64_t(void*)>;

}
