#pragma once

#include "dal/util/model_data.h"


namespace dal {

    class StaticMeshBuilder {

    private:
        dal::Mesh<dal::VertexStatic> m_output;

    public:
        auto& output_data() const {
            return this->m_output;
        }

        void add_vertex(const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& uv_coord);

        // Returns true if the quad is flat
        bool add_quad(
            const glm::vec3& p01,
            const glm::vec3& p00,
            const glm::vec3& p10,
            const glm::vec3& p11
        );

    private:
        VertexStatic& new_vertex();

    };


    class IStaticMeshGenerator {

    public:
        virtual ~IStaticMeshGenerator() = default;

        virtual void generate_points(StaticMeshBuilder& output) const = 0;

        StaticMeshBuilder generate_points() const;

    };


    class QuadMeshGenerator : public IStaticMeshGenerator {

    private:
        glm::vec3 m_p01;
        glm::vec3 m_p00;
        glm::vec3 m_p10;
        glm::vec3 m_p11;

    public:
        QuadMeshGenerator(
            const glm::vec3& p01,
            const glm::vec3& p00,
            const glm::vec3& p10,
            const glm::vec3& p11
        );

        void generate_points(StaticMeshBuilder& output) const override;

    };

}
