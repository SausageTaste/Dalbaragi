#include "d_render_cpnt.h"

#include <glm/gtc/matrix_transform.hpp>


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
        const auto average_vec = ::calc_average_vec(begin, end);
        const auto view_mat = this->make_view_mat(average_vec);

        float xp = 0;
        float xn = 0;
        float yp = 0;
        float yn = 0;
        float zp = 0;
        float zn = 0;

        for (auto iter = begin; iter != end; ++iter) {
            const auto v = glm::vec3{ view_mat * glm::vec4{*iter, 1} };

            if (v.x > xp)
                xp = v.x;
            if (v.x < xn)
                xn = v.x;
            if (v.y > yp)
                yp = v.y;
            if (v.y < yn)
                yn = v.y;
            if (v.z > zp)
                zp = v.z;
            if (v.z < zn)
                zn = v.z;
        }

        auto proj_mat = glm::ortho<float>(xn, xp, yn, yp, zn - 10, zp + 10);
        proj_mat[1][1] *= -1;

        return proj_mat * view_mat;
    }

    glm::mat4 SLight::make_light_mat() const {
        auto proj_mat = glm::perspective<float>(this->m_fade_end_radians * 2, 1, 1, this->m_max_dist);
        proj_mat[1][1] *= -1;

        return proj_mat * this->make_view_mat(this->m_pos);
    }

}
