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

    float calc_plane_d(const glm::vec3& point, const glm::vec3& normal) {
        return -glm::dot(normal, point);
    }

    // Parameter `normal` must be normalized
    glm::mat4 make_reflect_mat(const glm::vec3& point, const glm::vec3& normal) {
        const auto cos_theta = glm::dot(normal, glm::vec3{0, 1, 0});
        if (1.0 == cos_theta)
            return ::make_upside_down_mat();

        const auto rotate_axis = glm::cross(normal, glm::vec3{0, 1, 0});
        const auto quat = glm::rotate(glm::quat{1, 0, 0, 0}, acos(cos_theta), rotate_axis);
        const auto rotation = glm::mat4_cast(quat);

        const auto reorient = rotation * glm::translate(glm::mat4{1}, -point);
        return glm::inverse(reorient) * ::make_upside_down_mat() * reorient;
    }

}


// Plane
namespace dal {

    static_assert(sizeof(float) * 4 == sizeof(dal::Plane));


    Plane::Plane(const glm::vec3 point, const glm::vec3 normal)
        : m_normal(glm::normalize(normal))
        , m_d(::calc_plane_d(point, this->m_normal))
    {

    }

    Plane::Plane(const glm::vec3 p0, const glm::vec3 p1, const glm::vec3 p2)
        : Plane(p0, ::calc_normal_ccw(p0, p1, p2))
    {

    }

    const glm::vec4& Plane::coeff() const {
        return *reinterpret_cast<const glm::vec4*>(this);
    }

    const glm::vec3& Plane::normal() const {
        return this->m_normal;
    }

    glm::vec3 Plane::one_point() const {
        const auto t = -this->calc_signed_dist(glm::vec3{0});
        return t * this->normal();
    }

    float Plane::calc_signed_dist(const glm::vec3& p) const {
        return glm::dot(this->coeff(), glm::vec4{ p, 1 });
    }

    glm::mat4 Plane::make_reflect_mat() const {
        return ::make_reflect_mat(this->one_point(), this->normal());
    }

}
