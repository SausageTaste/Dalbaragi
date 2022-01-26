#include "d_mesh_builder.h"


// StaticMeshBuilder
namespace dal {

    void StaticMeshBuilder::add_vertex(const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& uv_coord) {
        auto& vert = this->new_vertex();
        vert.m_pos = pos;
        vert.m_normal = normal;
        vert.m_uv_coord = uv_coord;
    }

    bool StaticMeshBuilder::add_quad(
        const glm::vec3& p01,
        const glm::vec3& p00,
        const glm::vec3& p10,
        const glm::vec3& p11
    ) {
        const auto normal0 = glm::normalize(glm::cross(p00 - p01, p10 - p01));
        const auto normal1 = glm::normalize(glm::cross(p11 - p10, p01 - p10));

        this->add_vertex(p01, normal0, glm::vec2{0, 1});
        this->add_vertex(p00, normal0, glm::vec2{0, 0});
        this->add_vertex(p10, normal0, glm::vec2{1, 0});

        this->add_vertex(p01, normal1, glm::vec2{0, 1});
        this->add_vertex(p10, normal1, glm::vec2{1, 0});
        this->add_vertex(p11, normal1, glm::vec2{1, 1});

        return glm::dot(normal0, normal1) < 0.001f;
    }

    // Private

    VertexStatic& StaticMeshBuilder::new_vertex() {
        this->m_output.m_indices.push_back(this->m_output.m_indices.size());
        return m_output.m_vertices.emplace_back();
    }

}


// IMeshGenerator
namespace dal {

    StaticMeshBuilder IStaticMeshGenerator::generate_points() const {
        StaticMeshBuilder output;
        this->generate_points(output);
        return output;
    }

}


// QuadMeshGenerator
namespace dal {

    QuadMeshGenerator::QuadMeshGenerator(
        const glm::vec3& p01,
        const glm::vec3& p00,
        const glm::vec3& p10,
        const glm::vec3& p11
    )
        : m_p01(p01)
        , m_p00(p00)
        , m_p10(p10)
        , m_p11(p11)
    {

    }

    void QuadMeshGenerator::generate_points(StaticMeshBuilder& output) const {
        output.add_quad(this->m_p01, this->m_p00, this->m_p10, this->m_p11);
    }

}
