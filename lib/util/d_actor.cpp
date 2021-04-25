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
        _Mat output{1};

        const auto cx = cos(x);
        const auto sx = sin(x);
        const auto cy = cos(y);
        const auto sy = sin(y);

        output[0][0] = cy;
        output[0][1] = sx*sy;
        output[0][2] = -cx*sy;

        output[1][0] = 0;
        output[1][1] = cx;
        output[1][2] = sx;

        output[2][0] = sy;
        output[2][1] = -sx*cy;
        output[2][2] = cx*cy;

        return output;
    }

    template <typename _Mat>
    _Mat make_rotation_yx(const float x, const float y) {
        return glm::transpose(::make_rotation_xy<_Mat>(-x, -y));
    }

    template <typename _Mat>
    _Mat make_rotation_zxy(const float x, const float y, const float z) {
        _Mat output{1};

        const auto cx = cos(x);
        const auto sx = sin(x);
        const auto cy = cos(y);
        const auto sy = sin(y);
        const auto cz = cos(z);
        const auto sz = sin(z);

        output[0][0] = cz*cy - sz*sx*sy;
        output[0][1] = sz*cy + cz*sx*sy;
        output[0][2] = -cx*sy;

        output[1][0] = -sz*cx;
        output[1][1] = cz*cx;
        output[1][2] = sx;

        output[2][0] = cz*sy + sz*sx*cy;
        output[2][1] = sz*sy - cz*sx*cy;
        output[2][2] = cx*cy;

        return output;
    }

    template <typename _Mat>
    _Mat make_rotation_zxy(const glm::vec3& v) {
        return ::make_rotation_zxy<_Mat>(v.x, v.y, v.z);
    }

    template <typename _Mat>
    _Mat make_rotation_yxz(const glm::vec3& v) {
        return glm::transpose(::make_rotation_zxy<_Mat>(-v));
    }

}


namespace dal {

    glm::mat4 EulerCamera::make_view_mat() const {
        const auto translate = glm::translate(glm::mat4{1}, -this->m_pos);
        const auto rotation = ::make_rotation_zxy<glm::mat4>(-this->m_rotations);
        return rotation * translate;
    }

    void EulerCamera::move_horizontal(glm::vec3 v) {
        v.y = 0;
        const auto rotated = make_rotation_y<glm::mat3>(this->m_rotations.y) * v;

        this->m_pos.x += rotated.x;
        this->m_pos.z += rotated.z;
    }

    void EulerCamera::move_forward(const glm::vec3& v) {
        this->m_pos += ::make_rotation_yxz<glm::mat3>(this->m_rotations) * v;
    }

}
