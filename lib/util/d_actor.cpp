#include "d_actor.h"


namespace {

    template <typename _Mat>
    _Mat make_rotation_x(const float radians) {
        _Mat output{1};

        const auto c = cos(radians);
        const auto s = sin(radians);

        output[1][1] = c;
        output[1][2] = s;

        output[2][1] = -s;
        output[2][2] = c;

        return output;
    }

    template <typename _Mat>
    _Mat make_rotation_y(const float radians) {
        _Mat output{1};

        const auto c = cos(radians);
        const auto s = sin(radians);

        output[0][0] = c;
        output[0][2] = -s;

        output[2][0] = s;
        output[2][2] = c;

        return output;
    }

    template <typename _Mat>
    _Mat make_rotation_z(const float radians) {
        _Mat output{1};

        const auto c = cos(radians);
        const auto s = sin(radians);

        output[0][0] = c;
        output[0][1] = s;

        output[1][0] = -s;
        output[1][1] = c;

        return output;
    }

    template <typename _Mat>
    _Mat make_rotation_xy(const float x, const float y) {
        return ::make_rotation_x<_Mat>(x) * ::make_rotation_y<_Mat>(y);
    }

    template <typename _Mat>
    _Mat make_rotation_zxy(const float x, const float y, const float z) {
        return ::make_rotation_z<_Mat>(z) * ::make_rotation_xy<_Mat>(x, y);
    }

    template <typename _Mat>
    _Mat make_rotation_zxy(const glm::vec3& v) {
        return ::make_rotation_zxy<_Mat>(v.x, v.y, v.z);
    }

}


namespace dal {

    glm::mat4 EulerCamera::make_view_mat() const {
        const auto translate = glm::translate(glm::mat4{1}, -this->m_pos);
        const auto rotation = ::make_rotation_zxy<glm::mat4>(-this->m_rotations);
        return rotation * translate;
    }

    void EulerCamera::move_horizontal(const float x, const float z) {
        const glm::vec4 move_vec{ x, 0, z, 0 };
        const auto rotated = glm::rotate(glm::mat4{1}, this->m_rotations.y, glm::vec3{0, 1, 0}) * move_vec;

        this->m_pos.x += rotated.x;
        this->m_pos.z += rotated.z;
    }

    void EulerCamera::move_forward(const glm::vec3& v) {
        // Transpose of rotation matrices are equal to inverse of them
        this->m_pos += ::make_rotation_zxy<glm::mat3>(this->m_rotations) * v;
    }

}
