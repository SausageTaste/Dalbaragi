#include "d_render_cpnt.h"

#include <glm/gtc/matrix_transform.hpp>


namespace dal {

    glm::mat4 IDirectionalLight::make_view_mat(const glm::vec3& pos) const {
        return glm::lookAt(this->to_light_direc() + pos, pos, glm::vec3{0, 1, 0});
    }

    glm::mat4 DLight::make_light_mat(const float half_proj_box_edge_length) const {
        auto proj_mat = glm::ortho<float>(
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length * 10, half_proj_box_edge_length - 10
        );
        proj_mat[1][1] *= -1;

        return proj_mat * this->make_view_mat(this->m_pos);
    }

    glm::mat4 SLight::make_light_mat() const {
        auto proj_mat = glm::perspective<float>(this->m_fade_end_radians * 2, 1, 1, this->m_max_dist);
        proj_mat[1][1] *= -1;

        return proj_mat * this->make_view_mat(this->m_pos);
    }

}
