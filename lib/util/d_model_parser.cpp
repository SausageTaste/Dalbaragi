#include "d_model_parser.h"

#include <fmt/format.h>
#include <dal_model_parser.h>
#include <dal_modifier.h>

#include "d_logger.h"


namespace dal {

    bool parse_model_dmd(ModelStatic& output, const uint8_t* const data, const size_t data_size) {
        const auto unzipped = parser::unzip_dmd(data, data_size);
        if (!unzipped)
            return false;

        const auto model_data = parser::parse_dmd(unzipped->data(), unzipped->size());
        if (!model_data)
            return false;

        for (const auto& unit : model_data->m_units_indexed) {
            auto& output_render_unit = output.m_units.emplace_back();

            static_assert(sizeof(dal::parser::Vertex) == sizeof(dal::VertexStatic));
            static_assert(offsetof(dal::parser::Vertex, m_position) == offsetof(dal::VertexStatic, m_pos));
            static_assert(offsetof(dal::parser::Vertex, m_normal) == offsetof(dal::VertexStatic, m_normal));
            static_assert(offsetof(dal::parser::Vertex, m_uv_coords) == offsetof(dal::VertexStatic, m_uv_coord));
            output_render_unit.m_vertices.resize(unit.m_mesh.m_vertices.size());
            memcpy(output_render_unit.m_vertices.data(), unit.m_mesh.m_vertices.data(), output_render_unit.m_vertices.size() * sizeof(dal::VertexStatic));

            output_render_unit.m_indices.assign(unit.m_mesh.m_indices.begin(), unit.m_mesh.m_indices.end());

            output_render_unit.m_material.m_albedo_map = unit.m_material.m_albedo_map;
            output_render_unit.m_material.m_roughness = unit.m_material.m_roughness;
            output_render_unit.m_material.m_metallic = unit.m_material.m_metallic;
            output_render_unit.m_material.m_alpha_blending = unit.m_material.alpha_blend;
        }

        for (const auto& unit : model_data->m_units_indexed_joint) {
            auto& output_render_unit = output.m_units.emplace_back();

            output_render_unit.m_vertices.resize(unit.m_mesh.m_vertices.size());
            for (size_t i = 0; i < output_render_unit.m_vertices.size(); ++i) {
                output_render_unit.m_vertices[i].m_pos = unit.m_mesh.m_vertices[i].m_position;
                output_render_unit.m_vertices[i].m_normal = unit.m_mesh.m_vertices[i].m_normal;
                output_render_unit.m_vertices[i].m_uv_coord = unit.m_mesh.m_vertices[i].m_uv_coords;
            }

            output_render_unit.m_indices.assign(unit.m_mesh.m_indices.begin(), unit.m_mesh.m_indices.end());

            output_render_unit.m_material.m_albedo_map = unit.m_material.m_albedo_map;
            output_render_unit.m_material.m_roughness = unit.m_material.m_roughness;
            output_render_unit.m_material.m_metallic = unit.m_material.m_metallic;
            output_render_unit.m_material.m_alpha_blending = unit.m_material.alpha_blend;
        }

        if (!model_data->m_units_straight.empty())
            dalWarn("Not supported vertex data: straight");
        if (!model_data->m_units_straight_joint.empty())
            dalWarn("Not supported vertex data: straight joint");

        return true;
    }

    std::optional<ModelStatic> parse_model_dmd(const uint8_t* const data, const size_t data_size) {
        ModelStatic output;
        if (true != parse_model_dmd(output, data, data_size)) {
            return std::nullopt;
        }
        else {
            return output;
        }
    }

}
