#include "d_render_cpnt.h"

#include <glm/gtc/matrix_transform.hpp>


namespace dal {

    glm::mat4 DLight::make_light_mat(const float half_proj_box_edge_length) const {
        const auto view_mat = glm::lookAt(this->to_light_direc() + this->m_pos, this->m_pos, glm::vec3{0, 1, 0});

        auto proj_mat = glm::ortho<float>(
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length, half_proj_box_edge_length,
            -half_proj_box_edge_length * 10, half_proj_box_edge_length - 10
        );
        proj_mat[1][1] *= -1;

        return proj_mat * view_mat;
    }

}