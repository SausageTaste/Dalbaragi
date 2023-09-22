#include "d_render_cpnt.h"

#include <limits>

#include <fmt/format.h>

#include <daltools/util.h>

#include "d_logger.h"


namespace {

    template <typename _Iter>
    glm::vec3 calc_average_vec(const _Iter begin, const _Iter end) {
        if (begin == end)
            return glm::vec3{ 0 };

        glm::vec3 sum_vec{0};
        uint32_t vec_count = 0;

        for (auto iter = begin; iter != end; ++iter) {
            const glm::vec3& v = *iter;
            sum_vec += v;
            ++vec_count;
        }

        return sum_vec / static_cast<float>(vec_count);
    }

}


namespace dal {

    glm::mat4 IDirectionalLight::make_view_mat(const glm::vec3& pos) const {
        return glm::lookAt(this->to_light_direc() + pos, pos, glm::vec3{0, 1, 0});
    }

    glm::mat4 DLight::make_light_mat(const float half_proj_box_edge_length) const {
        auto proj_mat = glm::ortho<float>(
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length * 10, half_proj_box_edge_length * 10
        );
        proj_mat[1][1] *= -1;

        return proj_mat * this->make_view_mat(this->m_pos);
    }

    glm::mat4 DLight::make_light_mat(const glm::vec3* const begin, const glm::vec3* const end) const {
        const auto view_mat = this->make_view_mat(glm::vec3{0});

        glm::vec3 min{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
        glm::vec3 max = -min;

        for (auto iter = begin; iter != end; ++iter) {
            const auto v = view_mat * glm::vec4{*iter, 1};

            for (int i = 0; i < 3; ++i) {
                max[i] = std::max(v[i], max[i]);
                min[i] = std::min(v[i], min[i]);
            }
        }

        /*
        const auto cos_t = (cos(get_cur_sec()) + 1.0) * 0.5;
        const auto sin_t = (sin(get_cur_sec()) + 1.0) * 0.5;
        const auto t_mod = std::fmod(get_cur_sec(), 1);
        const auto either_0_1 = std::floor(std::fmod(get_cur_sec(), 6.0)) / 5.0;
        */

        auto proj_mat = glm::ortho<float>(min.x, max.x, -max.y, -min.y, -2.f * max.z + min.z, -min.z);  // Why the hell???
        proj_mat[1][1] *= -1;

        //dalInfo(fmt::format("{:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}, {:.2f}", min.x, max.x, min.y, max.y, min.z, max.z).c_str());

        return proj_mat * view_mat;
    }

    glm::mat4 SLight::make_light_mat() const {
        auto proj_mat = glm::perspective<float>(this->m_fade_end_radians * 2, 1, 1, this->m_max_dist);
        proj_mat[1][1] *= -1;

        return proj_mat * this->make_view_mat(this->m_pos);
    }

}
