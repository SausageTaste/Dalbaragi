#pragma once

#include <vector>

#include "d_geometry.h"


namespace dal {

    class TriangleSoup {

    public:
        glm::mat4 m_transformation;

    private:
        std::vector<Triangle> m_triangles;

    public:
        auto& emplace_back(const Triangle& tri) {
            return this->m_triangles.emplace_back(tri);
        }

        auto& new_triangle() {
            return this->m_triangles.emplace_back();
        }

        bool is_intersecting(const Segment& seg) const;

        std::optional<SegmentIntersectionInfo> find_intersection(const Segment& seg) const;

    };

}
