#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class Plane {

    private:
        glm::vec3 m_normal{ 0, 1, 0 };
        float m_d = 0;

    public:
        Plane() = default;

        Plane(const glm::vec3 point, const glm::vec3 normal);

        Plane(const glm::vec3 p0, const glm::vec3 p1, const glm::vec3 p2);

        const glm::vec4& coeff() const;

        const glm::vec3& normal() const;

        glm::vec3 one_point() const;

        float calc_signed_dist(const glm::vec3& p) const;

        glm::mat4 make_reflect_mat() const;

    };

}
