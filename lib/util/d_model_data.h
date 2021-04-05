#pragma once

#include <vector>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    struct VertexStatic {
        glm::vec3 m_pos;
        glm::vec3 m_normal;
        glm::vec2 m_uv_coord;
    };

    struct Material {
        std::string m_albedo_map;
        float m_roughness;
        float m_metallic;
    };

    struct RenderUnitStatic {
        std::vector<VertexStatic> m_vertices;
        std::vector<uint32_t> m_indices;
        Material m_material;
    };

    struct ModelStatic {
        std::vector<RenderUnitStatic> m_units;
    };


    bool make_static_mesh_aabb(RenderUnitStatic& output, const glm::vec3 min, const glm::vec3 max);

    RenderUnitStatic make_static_mesh_aabb(const glm::vec3 min, const glm::vec3 max);

}
