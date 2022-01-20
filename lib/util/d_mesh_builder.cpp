#include "d_mesh_builder.h"


namespace {

    class StaticMeshBuilder {

    private:
        dal::Mesh<dal::VertexStatic> output;

    public:
        auto& new_vertex() {
            this->output.m_indices.push_back(this->output.m_indices.size());
            return output.m_vertices.emplace_back();
        }

    };

}


// IMeshGenerator
namespace dal {

    Mesh<VertexStatic> IMeshGenerator::generate_points() const {
        Mesh<VertexStatic> output;
        this->generate_points(output);
        return output;
    }

}
