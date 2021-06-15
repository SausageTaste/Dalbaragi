#pragma once

#include "d_animation.h"


namespace dal {

    struct VertexStatic {
        glm::vec3 m_pos;
        glm::vec3 m_normal;
        glm::vec2 m_uv_coord;
    };

    struct VertexSkinned {
        glm::ivec4 m_joint_ids;
        glm::vec4 m_joint_weights;
        glm::vec3 m_pos;
        glm::vec3 m_normal;
        glm::vec2 m_uv_coord;
    };

    struct Material {
        std::string m_albedo_map;
        float m_roughness;
        float m_metallic;
        bool m_alpha_blending;
    };

    template <typename _Vertex>
    struct TRenderUnit {
        std::vector<_Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        Material m_material;
        glm::vec3 m_weight_center;
    };

    using RenderUnitStatic = TRenderUnit<VertexStatic>;
    using RenderUnitSkinned = TRenderUnit<VertexSkinned>;


    struct ModelStatic {
        std::vector<RenderUnitStatic> m_units;
    };

    struct ModelSkinned {
        std::vector<RenderUnitSkinned> m_units;
        std::vector<Animation> m_animations;
    };


    bool make_static_mesh_aabb(RenderUnitStatic& output, const glm::vec3 min, const glm::vec3 max, const glm::vec2 uv_scale);

    RenderUnitStatic make_static_mesh_aabb(const glm::vec3 min, const glm::vec3 max, const glm::vec2 uv_scale);

}
