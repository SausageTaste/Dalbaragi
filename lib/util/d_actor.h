#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class Transform {

    public:
        glm::quat m_quat{1, 0, 0, 0};
        glm::vec3 m_pos{0, 0, 0};
        float m_scale = 1;

    public:
        glm::mat4 make_mat4() const;

        void rotate(const float v, const glm::vec3& selector);

    };


    class ICamera {

    public:
        virtual ~ICamera() = default;

        virtual glm::vec3 view_pos() const = 0;

        virtual glm::mat4 make_view_mat() const = 0;

    };


    class EulerCamera : public ICamera {

    public:
        glm::vec3 m_pos{}, m_rotations{};

    public:
        glm::vec3 view_pos() const override {
            return this->m_pos;
        }

        glm::mat4 make_view_mat() const override;

        // -z is forward, +x is right, y is discarded
        void move_horizontal(glm::vec3 v);

        // -z is forward, +y is up, +x is right
        void move_forward(const glm::vec3& v);

    };

}
