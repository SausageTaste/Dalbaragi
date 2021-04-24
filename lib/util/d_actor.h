#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class Transform {

    public:
        glm::quat m_quat{};
        glm::vec3 m_pos{};
        float m_scale = 1;

    };


    class EulerCamera {

    public:
        glm::vec3 m_pos{}, m_rotations{};

    public:
        auto& view_pos() const {
            return this->m_pos;
        }

        glm::mat4 make_view_mat() const;

        // -z is forward, +x is right
        void move_horizontal(const float x, const float z);

        // -z is forward, +y is up, +x is right
        void move_forward(const glm::vec3& v);

    };

}
