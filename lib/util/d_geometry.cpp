#include "d_geometry.h"


namespace {

    glm::mat4 make_upside_down_mat() {
        glm::mat4 output{1};
        output[1][1] = -1;
        return output;
    }

    glm::vec3 calc_normal_ccw(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
        return glm::cross(p1 - p0, p2 - p0);
    }

}


// Plane
namespace dal {

    Plane::Plane(const glm::vec3 point, const glm::vec3 normal)
        : m_normal(glm::normalize(normal))
        , m_d(-glm::dot(normal, point))
    {

    }

    Plane::Plane(const glm::vec3 p0, const glm::vec3 p1, const glm::vec3 p2)
        : Plane(p0, ::calc_normal_ccw(p0, p1, p2))
    {

    }

    glm::vec3 Plane::one_point() const {
        return this->normal() * this->calc_signed_dist_to(glm::vec3{0});
    }

    float Plane::calc_signed_dist_to(const glm::vec3& p) const {
        return glm::dot(this->coeff(), glm::vec4{ p, 1 });
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
