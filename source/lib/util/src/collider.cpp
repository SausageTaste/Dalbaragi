#include "dal/util/collider.h"


namespace dal {

    bool TriangleSoup::is_intersecting(const Segment& seg) const {
        return std::nullopt != this->find_intersection(seg);
    }

    std::optional<SegmentIntersectionInfo> TriangleSoup::find_intersection(const Segment& seg) const {
        for (auto& x : this->m_triangles) {
            const auto new_tri = x.transform(this->m_transformation);
            const auto result = new_tri.find_intersection(seg, true);

            if (result.has_value())
                return result;
        }

        return std::nullopt;
    }

}
