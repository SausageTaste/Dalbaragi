#include "d_geometry.h"


namespace {

    glm::mat4 make_upside_down_mat() {
        glm::mat4 output{1};
        output[1][1] = -1;
        return output;
    }

}


// Plane
namespace dal {

    Plane::Plane(const glm::vec3 point, const glm::vec3 normal)
        : m_point(point)
        , m_normal(glm::normalize(normal))
    {

    }

    glm::mat4 Plane::make_reflect_mat() const {
        const auto upside_down = ::make_upside_down_mat();

        const auto cos_theta = glm::dot(this->normal(), glm::vec3{0, 1, 0});
        if (1.0 == cos_theta) {
            return upside_down;
        }

        const auto translation = glm::translate(glm::mat4{1}, -this->one_point());

        const auto rotate_axis = glm::cross(this->normal(), glm::vec3{0, 1, 0});
        const auto theta = acos(cos_theta);
        const auto q = glm::rotate(glm::quat{1, 0, 0, 0}, theta, rotate_axis);
        const auto rotation = glm::mat4_cast(q);

        return glm::inverse(translation) * glm::inverse(rotation) * upside_down * rotation * translation;
    }

}
