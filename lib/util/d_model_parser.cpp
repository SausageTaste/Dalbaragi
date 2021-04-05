#include "d_model_parser.h"

#include <fmt/format.h>
#include <dal_model_parser.h>
#include <dal_modifier.h>

#include "d_logger.h"


namespace dal {

    bool parse_model_dmd(ModelStatic& output, const uint8_t* const data, const size_t data_size) {
        const auto model_data = parser::parse_model_straight(data, data_size);
        if (!model_data) {
            return false;
        }

        for (const auto& x : model_data->m_render_units) {
            assert(x.m_mesh.m_vertices.size() % 9 == 0);

            const auto indexed_mesh = parser::convert_to_indexed(x.m_mesh);
            auto& output_render_unit = output.m_units.emplace_back();

            for (const auto& vert : indexed_mesh.m_vertices) {
                auto& fitted_vert = output_render_unit.m_vertices.emplace_back();
                fitted_vert.m_pos = vert.m_position;
                fitted_vert.m_uv_coord = vert.m_uv_coords;
                fitted_vert.m_normal = vert.m_normal;

                fitted_vert.m_uv_coord.y = 1.f - fitted_vert.m_uv_coord.y;
            }

            for (const auto& index : indexed_mesh.m_indices) {
                output_render_unit.m_indices.emplace_back(index);
            }

            {
                output_render_unit.m_material.m_albedo_map = x.m_material.m_albedo_map;
                output_render_unit.m_material.m_roughness = x.m_material.m_roughness;
                output_render_unit.m_material.m_metallic = x.m_material.m_metallic;
            }
        }

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
