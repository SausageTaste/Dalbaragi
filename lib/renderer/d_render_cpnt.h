#pragma once

#include <array>
#include <vector>
#include <memory>

#include "d_actor.h"
#include "d_animation.h"
#include "d_model_data.h"
#include "d_image_parser.h"


// Lights
namespace dal {

    class ILight {

    public:
        glm::vec3 m_pos;
        glm::vec3 m_color;

    };


    class IDirectionalLight {

    private:
        glm::vec3 m_direc_to_light;

    public:
        auto& to_light_direc() const {
            return this->m_direc_to_light;
        }

        void set_direc_to_light(const glm::vec3& v) {
            this->m_direc_to_light = glm::normalize(v);
        }

        void set_direc_to_light(const float x, const float y, const float z) {
            this->set_direc_to_light(glm::vec3{x, y, z});
        }

        glm::mat4 make_view_mat(const glm::vec3& pos) const;

    };


    class IDistanceLight {

    public:
        double m_max_dist = 20;

    };


    class DLight : public ILight, public IDirectionalLight {

    public:
        float m_atmos_intensity;

    public:
        glm::mat4 make_light_mat(const float half_proj_box_edge_length) const;

        glm::mat4 make_light_mat(const glm::vec3* const begin, const glm::vec3* const end) const;

    };


    class PLight : public ILight, public IDistanceLight {

    };


    class SLight : public ILight, public IDirectionalLight, public IDistanceLight {

    private:
        double m_fade_start;
        double m_fade_end;
        double m_fade_end_radians;

    public:
        auto fade_start() const {
            return this->m_fade_start;
        }

        auto fade_end() const {
            return this->m_fade_end;
        }

        auto fade_end_radians() const {
            return this->m_fade_end_radians;
        }

        void set_fade_start_degree(const double degree) {
            this->m_fade_start = std::cos(glm::radians(degree));
        }

        void set_fade_end_degree(const double degree) {
            this->m_fade_end_radians = glm::radians(degree);
            this->m_fade_end = std::cos(this->m_fade_end_radians);
        }

        glm::mat4 make_light_mat() const;

    };

}


// Abstract classes
namespace dal {

    class ITexture {

    public:
        virtual ~ITexture() = default;

        virtual bool set_image(const dal::ImageData& img_data) = 0;

        virtual void destroy() = 0;

        virtual bool is_ready() const = 0;

    };


    class IMesh {

    public:
        virtual ~IMesh() = default;

        virtual bool init_mesh(const std::vector<VertexStatic>& vertices, const std::vector<uint32_t>& indices) = 0;

        virtual void destroy() = 0;

        virtual bool is_ready() const = 0;

    };


    class IRenModel {

    public:
        virtual ~IRenModel() = default;

        virtual bool init_model(const dal::ModelStatic& model_data, const char* const fallback_namespace) = 0;

        virtual bool prepare() = 0;

        virtual void destroy() = 0;

        virtual bool is_ready() const = 0;

    };


    class IRenModelSkineed {

    public:
        virtual ~IRenModelSkineed() = default;

        virtual bool init_model(const dal::ModelSkinned& model_data, const char* const fallback_namespace) = 0;

        virtual bool prepare() = 0;

        virtual void destroy() = 0;

        virtual bool is_ready() const = 0;

        virtual std::vector<Animation>& animations() = 0;

        virtual const std::vector<Animation>& animations() const = 0;

        virtual const SkeletonInterface& skeleton() const = 0;

    };


    class IActor {

    public:
        Transform m_transform;

    public:
        virtual ~IActor() = default;

        virtual bool init() = 0;

        virtual void destroy() = 0;

        virtual bool is_ready() const = 0;

        virtual void notify_transform_change() = 0;

    };


    class IActorSkinned {

    public:
        dal::Transform m_transform;
        dal::AnimationState m_anim_state;

    public:
        virtual ~IActorSkinned() = default;

        virtual bool init() = 0;

        virtual void destroy() = 0;

        virtual bool is_ready() const = 0;

        virtual void notify_transform_change() = 0;

    };


    using HTexture = std::shared_ptr<ITexture>;
    using HMesh = std::shared_ptr<IMesh>;
    using HRenModel = std::shared_ptr<IRenModel>;
    using HRenModelSkinned = std::shared_ptr<IRenModelSkineed>;
    using HActor = std::shared_ptr<IActor>;
    using HActorSkinned = std::shared_ptr<IActorSkinned>;

}
