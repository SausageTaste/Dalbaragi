#pragma once

#include <array>
#include <optional>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class Segment {

    public:
        glm::vec3 m_start;
        glm::vec3 m_direc;

    public:
        Segment() = default;

        Segment(glm::vec3 start, glm::vec3 direc)
            : m_start(start)
            , m_direc(direc)
        {

        }

        glm::vec3 end_point() const {
            return this->m_start + this->m_direc;
        }

        float length_sqr() const {
            return glm::dot(this->m_direc, this->m_direc);
        }

        float length() const {
            return glm::length(this->m_direc);
        }

        Segment transform(const glm::mat4& mat) const;

    };

    class SegmentIntersectionInfo {

    public:
        float m_distance;
        bool m_from_front;

    public:
        SegmentIntersectionInfo(const float dist, const bool from_front)
            : m_distance(dist)
            , m_from_front(from_front)
        {

        }

        bool is_intersecting(const Segment& seg, const bool regard_direction) const {
            if (regard_direction && !this->m_from_front)
                return false;

            else if (this->m_distance < 0.f)
                return false;
            else if (this->m_distance > seg.length())
                return false;
            else
                return true;
        }

        glm::vec3 calc_intersecting_point(const Segment& seg) const {
            return seg.m_start + glm::normalize(seg.m_direc) * this->m_distance;
        }

    };


    class Plane {

    private:
        glm::vec3 m_normal{ 0, 1, 0 };
        float m_d = 0;

    public:
        Plane() = default;

        Plane(const glm::vec3& point, const glm::vec3& normal);

        Plane(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);

        const glm::vec4& coeff() const;

        const glm::vec3& normal() const;

        glm::vec3 one_point() const;

        float calc_signed_dist(const glm::vec3& p) const;

        glm::vec3 find_closest_point(const glm::vec3& p) const;

        std::optional<SegmentIntersectionInfo> find_intersection(const Segment& seg) const;

        glm::mat4 make_origin_align_mat() const;

        glm::mat4 make_reflect_mat() const;

    };


    class PlaneOriented {

    private:
        Plane m_plane;
        glm::vec3 m_center_point;
        glm::vec3 m_top_point;
        glm::vec3 m_winding_point;

    public:
        PlaneOriented() = default;

        PlaneOriented(const glm::vec3& center_point, const glm::vec3& top_point, const glm::vec3& winding_point);

        const glm::vec3& center_point() const {
            return this->m_center_point;
        }

        const glm::vec3& top_point() const {
            return this->m_top_point;
        }

        const glm::vec3& winding_point() const {
            return this->m_winding_point;
        }

        const dal::Plane& plane() const {
            return this->m_plane;
        }

        const glm::vec4& coeff() const {
            return this->plane().coeff();
        }

        const glm::vec3& normal() const {
            return this->plane().normal();
        }

        glm::mat4 make_origin_align_mat() const;

    };


    glm::mat4 make_upside_down_mat();

    glm::mat4 make_portal_mat(const PlaneOriented& mesh, const PlaneOriented& view);


    class Triangle {

    public:
        std::array<glm::vec3, 3> m_vertices;

    public:
        Triangle() = default;

        Plane make_plane() const {
            return Plane{ this->m_vertices[0], this->m_vertices[1], this->m_vertices[2] };
        }

        PlaneOriented make_plane_oriented() const{
            return PlaneOriented{ this->m_vertices[0], this->m_vertices[1], this->m_vertices[2] };
        }

        std::optional<SegmentIntersectionInfo> find_intersection(const Segment& seg, const bool ignore_from_back = false) const;

    };

}
