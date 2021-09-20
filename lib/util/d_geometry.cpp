#include "d_geometry.h"


namespace {

    const glm::vec3 LYING_PLANE_LOOK_DIREC{ 0, 1, 0 };
    const glm::vec3 LYING_PLANE_HEAD_DIREC{ 0, 0, -1 };


    float safe_acos(const float x) {
        return std::acos(glm::clamp<float>(x, -1, 1));
    }

    glm::mat4 roate_about_axis(const float radians, const glm::vec3& axis) {
        const auto quat = glm::rotate(glm::quat{1, 0, 0, 0}, radians, axis);
        return glm::mat4_cast(quat);
    }

    glm::mat4 make_origin_align_mat(const glm::vec3& point, const glm::vec3& normal) {
        glm::mat4 output{1};

        const auto cos_theta = glm::dot(normal, ::LYING_PLANE_LOOK_DIREC);
        if (1.f > cos_theta) {
            const auto rotate_axis = glm::cross(normal, ::LYING_PLANE_LOOK_DIREC);
            output *= ::roate_about_axis(::safe_acos(cos_theta), rotate_axis);
        }

        output *= glm::translate(glm::mat4{1}, -point);
        return output;
    }

}


namespace dal {

    glm::mat4 make_upside_down_mat() {
        glm::mat4 output{1};
        output[1][1] = -1;
        return output;
    }

    glm::mat4 make_portal_mat(const PlaneOriented& mesh, const PlaneOriented& view) {
        glm::mat4 output{1};

        output *= glm::inverse(mesh.make_origin_align_mat());
        output *= ::roate_about_axis(glm::radians<float>(180), ::LYING_PLANE_HEAD_DIREC);
        output *= view.make_origin_align_mat();

        return output;
    }

}


// Plane
namespace dal {

    static_assert(sizeof(float) * 4 == sizeof(dal::Plane));


    Plane::Plane(const glm::vec3& point, const glm::vec3& normal)
        : m_normal(glm::normalize(normal))
        , m_d(-glm::dot(point, this->m_normal))
    {

    }

    Plane::Plane(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2)
        : Plane(p0, glm::cross(p1 - p0, p2 - p0))
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

    glm::mat4 Plane::make_origin_align_mat() const {
        return ::make_origin_align_mat(this->one_point(), this->normal());
    }

    glm::mat4 Plane::make_reflect_mat() const {
        const auto oalign = this->make_origin_align_mat();
        return glm::inverse(oalign) * dal::make_upside_down_mat() * oalign;
    }

}


// PlaneOriented
namespace dal {

    PlaneOriented::PlaneOriented(const glm::vec3& center_point, const glm::vec3& top_point, const glm::vec3& winding_point)
        : m_plane(center_point, top_point, winding_point)
        , m_center_point(center_point)
        , m_top_point(top_point)
        , m_winding_point(winding_point)
    {

    }

    glm::mat4 PlaneOriented::make_origin_align_mat() const {
        glm::mat4 output{1};

        const auto oalign = ::make_origin_align_mat(this->center_point(), this->normal());
        const auto head_diect = glm::normalize(this->top_point() - this->center_point());
        const auto reoriented_head_diect = glm::mat3{oalign} * head_diect;
        const auto cos_theta = glm::dot(reoriented_head_diect, ::LYING_PLANE_HEAD_DIREC);

        if (1.f > cos_theta) {
            const auto rotate_axis = glm::cross(reoriented_head_diect, ::LYING_PLANE_HEAD_DIREC);
            output *= roate_about_axis(::safe_acos(cos_theta), rotate_axis);
        }

        output *= oalign;
        return output;
    }

}
