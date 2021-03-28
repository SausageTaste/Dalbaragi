#include "d_model_data.h"

#include <array>


namespace dal {

    RenderUnit make_aabb(const glm::vec3 min, const glm::vec3 max) {
        RenderUnit intermediate_data, result;

        intermediate_data.m_vertices = {
            { glm::vec3{min.x, min.y, min.z}, glm::normalize(glm::vec3{-1, -1, -1}), {1, 1} }, // 0
            { glm::vec3{min.x, min.y, max.z}, glm::normalize(glm::vec3{-1, -1,  1}), {0, 1} }, // 1
            { glm::vec3{max.x, min.y, max.z}, glm::normalize(glm::vec3{ 1, -1,  1}), {1, 1} }, // 2
            { glm::vec3{max.x, min.y, min.z}, glm::normalize(glm::vec3{ 1, -1, -1}), {0, 1} }, // 3
            { glm::vec3{min.x, max.y, min.z}, glm::normalize(glm::vec3{-1,  1, -1}), {1, 0} }, // 4
            { glm::vec3{min.x, max.y, max.z}, glm::normalize(glm::vec3{-1,  1,  1}), {0, 0} }, // 5
            { glm::vec3{max.x, max.y, max.z}, glm::normalize(glm::vec3{ 1,  1,  1}), {1, 0} }, // 6
            { glm::vec3{max.x, max.y, min.z}, glm::normalize(glm::vec3{ 1,  1, -1}), {0, 0} }, // 7
        };

        intermediate_data.m_indices = {
            0, 2, 1, 0, 3, 2,
            5, 1, 2, 5, 2, 6,
            6, 2, 3, 6, 3, 7,
            7, 3, 0, 7, 0, 4,
            4, 0, 1, 4, 1, 5,
            4, 5, 6, 4, 6, 7,
        };

        for (int i = 0; i < intermediate_data.m_indices.size() / 3; ++i) {
            const auto i0 = intermediate_data.m_indices.at(3 * i + 0);
            const auto i1 = intermediate_data.m_indices.at(3 * i + 1);
            const auto i2 = intermediate_data.m_indices.at(3 * i + 2);

            auto v0 = intermediate_data.m_vertices.at(i0);
            auto v1 = intermediate_data.m_vertices.at(i1);
            auto v2 = intermediate_data.m_vertices.at(i2);

            const auto edge1 = v1.m_pos - v0.m_pos;
            const auto edge2 = v2.m_pos - v0.m_pos;
            const auto normal = glm::normalize(glm::cross(edge1, edge2));

            v0.m_normal = normal;
            v1.m_normal = normal;
            v2.m_normal = normal;

            result.m_vertices.emplace_back(v0);
            result.m_vertices.emplace_back(v1);
            result.m_vertices.emplace_back(v2);

            result.m_indices.emplace_back(result.m_indices.size());
            result.m_indices.emplace_back(result.m_indices.size());
            result.m_indices.emplace_back(result.m_indices.size());
        }

        result.m_material.m_roughness = 0.2;
        result.m_material.m_metallic = 1;

        return result;
    }

}
