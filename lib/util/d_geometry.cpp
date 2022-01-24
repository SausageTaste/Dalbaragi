#include "d_geometry.h"


namespace {

    const glm::vec3 LYING_PLANE_LOOK_DIREC{ 0, 1, 0 };
    const glm::vec3 LYING_PLANE_HEAD_DIREC{ 0, 0, -1 };


    float safe_acos(const float x) {
        return std::acos(glm::clamp<float>(x, -1, 1));
    }

    glm::mat4 roate_about_axis_old(const float radians, const glm::vec3& axis) {
        const auto quat = glm::angleAxis(radians, glm::normalize(axis));
        return glm::mat4_cast(quat);
    }

    glm::mat4 roate_about_axis(const float radians, const glm::vec3& axis) {
        glm::mat4 output{1};

        const auto axis_n = glm::normalize(axis);

        const auto c = glm::cos(radians);
        const auto s = glm::sin(radians);
        const auto t = 1.f - c;
        const auto x = axis_n.x;
        const auto y = axis_n.y;
        const auto z = axis_n.z;

        output[0][0] = t*x*x + c;
        output[1][0] = t*x*y - z*s;
        output[2][0] = t*x*z + y*s;

        output[0][1] = t*x*y + z*s;
        output[1][1] = t*y*y + c;
        output[2][1] = t*y*z - x*s;

        output[0][2] = t*x*z - y*s;
        output[1][2] = t*y*z + x*s;
        output[2][2] = t*z*z + c;

        return output;
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

    // Param p must be on the plane.
    bool is_point_inside_triangle(const glm::vec3& p, const dal::Triangle& tri) {
        const auto edge1 = tri.m_vertices[1] - tri.m_vertices[0];
        const auto edge2 = tri.m_vertices[2] - tri.m_vertices[1];

        const auto toPoint1 = p - tri.m_vertices[0];
        const auto toPoint2 = p - tri.m_vertices[1];

        const auto crossed1 = glm::cross(edge1, toPoint1);
        const auto crossed2 = glm::cross(edge2, toPoint2);

        const auto dotted1 = glm::dot(crossed1, crossed2);

        if ( dotted1 < 0.f ) {
            return false;
        }

        const auto edge3 = tri.m_vertices[0] - tri.m_vertices[2];
        const auto toPoint3 = p - tri.m_vertices[2];
        const auto crossed3 = glm::cross(edge3, toPoint3);
        const auto dotted2 = glm::dot(crossed1, crossed3);

        if ( dotted2 < 0.f ) {
            return false;
        }

        return true;
    }

}


namespace dal {

    Segment Segment::transform(const glm::mat4& mat) const {
        Segment output;

        const auto new_st = mat * glm::vec4{this->m_start, 1};
        const auto new_ed = mat * glm::vec4{this->end_point(), 1};

        output.m_start = new_st;
        output.m_direc = new_ed - new_st;

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

    Plane Plane::transform(const glm::mat4& mat) const {
        const auto new_normal = mat * glm::vec4{ this->normal(), 0 };
        const auto new_point = mat * glm::vec4{ this->one_point(), 1 };
        return Plane{new_point, new_normal};
    }

    glm::vec3 Plane::one_point() const {
        const auto t = -this->calc_signed_dist(glm::vec3{0});
        return t * this->normal();
    }

    float Plane::calc_signed_dist(const glm::vec3& p) const {
        return glm::dot(this->coeff(), glm::vec4{ p, 1 });
    }

    glm::vec3 Plane::find_closest_point(const glm::vec3& p) const {
        const auto signed_dist = this->calc_signed_dist(p);
        return p - signed_dist * this->normal();
    }

    std::optional<SegmentIntersectionInfo> Plane::find_intersection(const Segment& seg) const {
        const auto pointA = seg.m_start;
        const auto pointB = seg.end_point();

        const auto distA = this->calc_signed_dist(pointA);
        const auto distB = this->calc_signed_dist(pointB);

        if ( (distA * distB) > 0.f )
            return std::nullopt;

        const auto absDistA = std::abs(distA);
        const auto denominator = absDistA + std::abs(distB);
        if ( 0.f == denominator )
            return SegmentIntersectionInfo{ 0, distA > distB };

        const auto distance = seg.length() * absDistA / denominator;
        return SegmentIntersectionInfo{ distance, distA > distB };
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

        if (-0.9999999f > cos_theta) {
            output *= roate_about_axis(::safe_acos(cos_theta), ::LYING_PLANE_LOOK_DIREC);
        }
        else if (1.f > cos_theta) {
            const auto rotate_axis = glm::cross(reoriented_head_diect, ::LYING_PLANE_HEAD_DIREC);
            output *= roate_about_axis(::safe_acos(cos_theta), rotate_axis);
        }

        output *= oalign;
        return output;
    }

}


// Triangle
namespace dal {

    std::optional<SegmentIntersectionInfo> Triangle::find_intersection(const Segment& seg, const bool ignore_from_back) const {
        const auto plane = this->make_plane();
        const auto plane_col = plane.find_intersection(seg);

        if (!plane_col)
            return std::nullopt;
        if (ignore_from_back && !plane_col->m_from_front)
            return std::nullopt;

        if (::is_point_inside_triangle(plane_col->calc_intersecting_point(seg), *this)) {
            return plane_col;
        }

        return std::nullopt;
    }

}
