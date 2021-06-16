#include "d_model_parser.h"

#include <fmt/format.h>
#include <dal_model_parser.h>
#include <dal_modifier.h>

#include "d_logger.h"


namespace {

    template <typename _VertType>
    glm::vec3 calc_weight_center(const std::vector<_VertType>& vertices) {
        double x_sum = 0.0;
        double y_sum = 0.0;
        double z_sum = 0.0;

        for (const auto& v : vertices) {
            x_sum += v.m_pos.x;
            y_sum += v.m_pos.y;
            z_sum += v.m_pos.z;
        }

        const double vert_count = static_cast<double>(vertices.size());

        return glm::vec3{
            x_sum / vert_count,
            y_sum / vert_count,
            z_sum / vert_count,
        };
    }

    void copy_material(dal::Material& dst, const dal::parser::Material& src) {
        dst.m_roughness = src.m_roughness;
        dst.m_metallic = src.m_metallic;
        dst.m_albedo_map = src.m_albedo_map;
        dst.m_alpha_blending = src.alpha_blend;
    }

}


namespace dal {

    bool parse_model_dmd(ModelStatic& output, const uint8_t* const data, const size_t data_size) {
        static_assert(sizeof(dal::parser::Vertex) == sizeof(dal::VertexStatic));
        static_assert(offsetof(dal::parser::Vertex, m_position) == offsetof(dal::VertexStatic, m_pos));
        static_assert(offsetof(dal::parser::Vertex, m_normal) == offsetof(dal::VertexStatic, m_normal));
        static_assert(offsetof(dal::parser::Vertex, m_uv_coords) == offsetof(dal::VertexStatic, m_uv_coord));

        const auto unzipped = parser::unzip_dmd(data, data_size);
        if (!unzipped)
            return false;

        const auto model_data = parser::parse_dmd(unzipped->data(), unzipped->size());
        if (!model_data)
            return false;

        for (const auto& unit : model_data->m_units_indexed) {
            auto& out_unit = output.m_units.emplace_back();

            out_unit.m_vertices.resize(unit.m_mesh.m_vertices.size());
            memcpy(out_unit.m_vertices.data(), unit.m_mesh.m_vertices.data(), out_unit.m_vertices.size() * sizeof(dal::VertexStatic));

            out_unit.m_indices.assign(unit.m_mesh.m_indices.begin(), unit.m_mesh.m_indices.end());
            ::copy_material(out_unit.m_material, unit.m_material);
            out_unit.m_weight_center = ::calc_weight_center(out_unit.m_vertices);
        }

        for (const auto& unit : model_data->m_units_indexed_joint) {
            auto& out_unit = output.m_units.emplace_back();

            out_unit.m_vertices.resize(unit.m_mesh.m_vertices.size());
            for (size_t i = 0; i < out_unit.m_vertices.size(); ++i) {
                auto& out_vert = out_unit.m_vertices[i];
                auto& in_vert = unit.m_mesh.m_vertices[i];

                out_vert.m_pos = in_vert.m_position;
                out_vert.m_normal = in_vert.m_normal;
                out_vert.m_uv_coord = in_vert.m_uv_coords;
            }

            out_unit.m_indices.assign(unit.m_mesh.m_indices.begin(), unit.m_mesh.m_indices.end());
            ::copy_material(out_unit.m_material, unit.m_material);
            out_unit.m_weight_center = ::calc_weight_center(out_unit.m_vertices);
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

    bool parse_model_skinned_dmd(ModelSkinned& output, const uint8_t* const data, const size_t data_size) {
        const auto unzipped = parser::unzip_dmd(data, data_size);
        if (!unzipped)
            return false;

        const auto model_data = parser::parse_dmd(unzipped->data(), unzipped->size());
        if (!model_data)
            return false;

        for (const auto& unit : model_data->m_units_indexed) {
            auto& out_unit = output.m_units.emplace_back();

            out_unit.m_vertices.resize(unit.m_mesh.m_vertices.size());
            for (size_t i = 0; i < out_unit.m_vertices.size(); ++i) {
                auto& out_vert = out_unit.m_vertices[i];
                auto& in_vert = unit.m_mesh.m_vertices[i];

                out_vert.m_joint_ids     = glm::ivec4{-1, -1, -1, -1};
                out_vert.m_joint_weights = glm::vec4{0, 0, 0, 0};
                out_vert.m_pos           = in_vert.m_position;
                out_vert.m_normal        = in_vert.m_normal;
                out_vert.m_uv_coord      = in_vert.m_uv_coords;
            }

            out_unit.m_indices.assign(unit.m_mesh.m_indices.begin(), unit.m_mesh.m_indices.end());
            ::copy_material(out_unit.m_material, unit.m_material);
            out_unit.m_weight_center = ::calc_weight_center(out_unit.m_vertices);
        }

        for (const auto& unit : model_data->m_units_indexed_joint) {
            auto& out_unit = output.m_units.emplace_back();

            out_unit.m_vertices.resize(unit.m_mesh.m_vertices.size());
            for (size_t i = 0; i < out_unit.m_vertices.size(); ++i) {
                auto& out_vert = out_unit.m_vertices[i];
                auto& in_vert = unit.m_mesh.m_vertices[i];

                out_vert.m_joint_ids     = in_vert.m_joint_indices;
                out_vert.m_joint_weights = in_vert.m_joint_weights;
                out_vert.m_pos           = in_vert.m_position;
                out_vert.m_normal        = in_vert.m_normal;
                out_vert.m_uv_coord      = in_vert.m_uv_coords;
            }

            out_unit.m_indices.assign(unit.m_mesh.m_indices.begin(), unit.m_mesh.m_indices.end());
            ::copy_material(out_unit.m_material, unit.m_material);
            out_unit.m_weight_center = ::calc_weight_center(out_unit.m_vertices);
        }

        for (auto& out_anim : model_data->m_animations) {
            auto& in_anim = output.m_animations.emplace_back(out_anim.m_name, out_anim.m_ticks_par_sec, out_anim.m_duration_tick);

            for (auto& out_joint : out_anim.m_joints) {
                auto& in_joint = in_anim.newJoint();

                in_joint.setName(out_joint.m_name);

                for (auto& out_translate : out_joint.m_translates)
                    in_joint.addPos(out_translate.first, out_translate.second);

                for (auto& out_rotation : out_joint.m_rotations)
                    in_joint.addRotation(out_rotation.first, out_rotation.second);

                for (auto& out_scale : out_joint.m_scales)
                    in_joint.addScale(out_scale.first, out_scale.second);
            }
        }

        for (auto& src_joint : model_data->m_skeleton.m_joints) {
            const auto jid = output.m_skeleton.get_or_make_index_of(src_joint.m_name);
            auto& dst_joint = output.m_skeleton.at(jid);
            dst_joint.set(src_joint);
        }

        if (output.m_skeleton.size() > 0) {
            // Character lies on ground without this line.
            output.m_skeleton.at(0).set_parent_mat(output.m_skeleton.at(0).offset());

            for ( int i = 1; i < output.m_skeleton.size(); ++i ) {
                auto& thisInfo = output.m_skeleton.at(i);
                const auto& parentInfo = output.m_skeleton.at(thisInfo.parent_index());
                thisInfo.set_parent_mat(parentInfo);
            }
        }

        if (!model_data->m_units_straight.empty())
            dalWarn("Not supported vertex data: straight");
        if (!model_data->m_units_straight_joint.empty())
            dalWarn("Not supported vertex data: straight joint");

        return true;
    }

    std::optional<ModelSkinned> parse_model_skinned_dmd(const uint8_t* const data, const size_t data_size) {
        ModelSkinned output;
        if (true != parse_model_skinned_dmd(output, data, data_size))
            return std::nullopt;
        else
            return output;
    }

}
