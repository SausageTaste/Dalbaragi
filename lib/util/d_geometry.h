#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class Plane {

    private:
        glm::vec3 m_point, m_normal;

    public:
        Plane(const glm::vec3 point, const glm::vec3 normal);

        glm::vec3 normal() const {
            return this->m_normal;
        }

        glm::vec3 one_point() const {
            return this->m_point;
        }

        glm::mat4 make_reflect_mat() const;

    };

}
