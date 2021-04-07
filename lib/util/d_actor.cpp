#include "d_actor.h"


namespace dal {

    glm::mat4 EulerCamera::make_view_mat() const {
        const auto identity = glm::mat4{1};
        const auto translate = glm::translate(identity, -this->m_pos);
        const auto rotation_x = glm::rotate(identity, -this->m_rotations.x, glm::vec3{1, 0, 0});
        const auto rotation_y = glm::rotate(identity, -this->m_rotations.y, glm::vec3{0, 1, 0});
        const auto rotation_z = glm::rotate(identity, -this->m_rotations.z, glm::vec3{0, 0, 1});

        return rotation_x * rotation_y * rotation_z * translate;
    }

    void EulerCamera::move_horizontal(const float x, const float z) {
        const glm::vec4 move_vec{ x, 0, z, 0 };
        const auto rotated = glm::rotate(glm::mat4{1}, this->m_rotations.y, glm::vec3{0, 1, 0}) * move_vec;

        this->m_pos.x += rotated.x;
        this->m_pos.z += rotated.z;
    }

}
