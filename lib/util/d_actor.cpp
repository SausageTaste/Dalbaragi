#include "d_actor.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <fmt/format.h>

#include "d_logger.h"
#include "d_geometry.h"


namespace {

    const auto GRAVITY_DIRECTION = glm::normalize(glm::vec3{ 0, -1, 0 });

    const auto DIRECTION_UP    = glm::normalize(glm::vec3{ 0, 1,  0 });
    const auto DIRECTION_FRONT = glm::normalize(glm::vec3{ 0, 0, -1 });
    const auto DIRECTION_RIGHT = glm::normalize(glm::vec3{ 1, 0,  0 });


    float mod_PI(float y) {
        constexpr float PI = static_cast<float>(M_PI);
        constexpr float PI_TIMES_2 = static_cast<float>(M_PI * 2.0);

        while (y > PI)
            y -= PI_TIMES_2;

        while (y < -PI)
            y += PI_TIMES_2;

        return y;
    }

    float clamp_HALF_PI(float y) {
        constexpr float PI = static_cast<float>(M_PI);
        constexpr float PI_HALF = static_cast<float>(M_PI * 0.5);
        constexpr float PI_TIMES_2 = static_cast<float>(M_PI * 2.0);

        if (y > PI_HALF)
            return PI_HALF;

        if (y < -PI_HALF)
            return -PI_HALF;

        return y;
    }


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

    template <typename _Mat>
    _Mat make_rotation_yxz(const float x, const float y, const float z) {
        return ::make_rotation_yxz<_Mat>(glm::vec3{x, y, z});
    }


    glm::quat rotate_quat(const glm::quat& q, const float radians, const glm::vec3& selector) {
        return glm::normalize(glm::angleAxis(radians, glm::normalize(selector)) * q);
    }


    auto decompose_rotations(const glm::mat4& m) {
        const auto sin_x = -m[2][1];
        const auto cos_x = sqrt((m[0][1] * m[0][1]) + (m[1][1] * m[1][1]));
        const auto tan_x = sin_x / cos_x;

        const auto x = atan2(sin_x, cos_x);
        const auto y = atan2(m[2][0], m[2][2]);
        const auto z = atan2(m[0][1], m[1][1]);

        return glm::vec3{ mod_PI(x), mod_PI(y), mod_PI(z) };
    }

}


// EulerAnglesYXZ
namespace dal {

    void EulerAnglesYXZ::set_x(const float v) {
        this->m_rotations.x = v;
        this->m_rotations.x = ::mod_PI(this->m_rotations.x);
    }

    void EulerAnglesYXZ::set_y(const float v) {
        this->m_rotations.y = v;
        this->m_rotations.y = ::mod_PI(this->m_rotations.y);
    }

    void EulerAnglesYXZ::set_z(const float v) {
        this->m_rotations.z = v;
        this->m_rotations.z = ::mod_PI(this->m_rotations.z);
    }

    void EulerAnglesYXZ::add_x(const float v) {
        this->m_rotations.x += v;
        this->m_rotations.x = ::mod_PI(this->m_rotations.x);
    }

    void EulerAnglesYXZ::add_y(const float v) {
        this->m_rotations.y += v;
        this->m_rotations.y = ::mod_PI(this->m_rotations.y);
    }

    void EulerAnglesYXZ::add_z(const float v) {
        this->m_rotations.z += v;
        this->m_rotations.z = ::mod_PI(this->m_rotations.z);
    }

    glm::mat4 EulerAnglesYXZ::make_view_mat() const {
        return ::make_rotation_zxy<glm::mat4>(-this->m_rotations);
    }

}


// Transform
namespace dal {

    // TODO: optimize this
    glm::mat3 Transform::make_mat3() const {
        const auto identity = glm::mat4{ 1 };
        const auto scale_mat = glm::scale(identity, glm::vec3{ this->m_scale, this->m_scale , this->m_scale });
        const auto translate_mat = glm::translate(identity, this->m_pos);

        return static_cast<glm::mat3>(translate_mat * glm::mat4_cast(this->m_quat) * scale_mat);
    }

    glm::mat4 Transform::make_mat4() const {
        const auto identity = glm::mat4{ 1 };
        const auto scale_mat = glm::scale(identity, glm::vec3{ this->m_scale, this->m_scale , this->m_scale });
        const auto translate_mat = glm::translate(identity, this->m_pos);

        return translate_mat * glm::mat4_cast(this->m_quat) * scale_mat;
    }

    void Transform::rotate(const float v, const glm::vec3& selector) {
        this->m_quat = ::rotate_quat(this->m_quat, v, selector);
    }

}


// ICamera
namespace dal {

    std::array<glm::vec3, 8> ICamera::make_frustum_vertices(const float fov, const float ratio, const float near, const float far) const {
        const auto view_inv = glm::inverse(this->make_view_mat());

        const float tan_half_angle_vertical = tanf(fov * 0.5);
        const float tan_half_angle_horizontal = tan_half_angle_vertical * ratio;

        const float half_width_near = near * tan_half_angle_horizontal;
        const float half_width_far = far * tan_half_angle_horizontal;
        const float half_height_near = near * tan_half_angle_vertical;
        const float half_height_far = far * tan_half_angle_vertical;

        std::array<glm::vec3, 8> new_output{
            glm::vec3{-half_width_near, -half_height_near, -near},
            glm::vec3{ half_width_near, -half_height_near, -near},
            glm::vec3{-half_width_near,  half_height_near, -near},
            glm::vec3{ half_width_near,  half_height_near, -near},
            glm::vec3{-half_width_far,  -half_height_far,  -far},
            glm::vec3{ half_width_far,  -half_height_far,  -far},
            glm::vec3{-half_width_far,   half_height_far,  -far},
            glm::vec3{ half_width_far,   half_height_far,  -far},
        };

        for (size_t i = 0; i < 8; ++i) {
            new_output[i] = view_inv * glm::vec4{new_output[i], 1};
        }

        return new_output;
    }

}


// EulerCamera
namespace dal {

    glm::mat4 EulerCamera::make_view_mat() const {
        const auto translate = glm::translate(glm::mat4{1}, -this->m_pos);
        const auto rotation = this->m_rotations.make_view_mat();
        const auto output = rotation * translate;
        return output;
    }

    void EulerCamera::set_rot_xyz(const float x, const float y, const float z) {
        this->m_rotations.set_xyz(x, y, z);
    }

    void EulerCamera::add_rot_xyz(const float x, const float y, const float z) {
        this->m_rotations.add_xyz(x, y, z);
    }

    void EulerCamera::rotate_head_up(const float delta_time) {
        if (0.f == this->m_rotations.z())
            return;

        const auto cur_z = this->m_rotations.z();
        const auto cur_z_abs = std::abs(cur_z);
        const auto z_delta = (-cur_z / cur_z_abs) * static_cast<float>(delta_time * 2.0);

        if (std::abs(z_delta) < cur_z_abs)
            this->m_rotations.add_z(z_delta);
        else
            this->m_rotations.set_z(0);
    }

    EulerCamera EulerCamera::transform(const glm::mat4& mat) const {
        EulerCamera output;

        output.m_pos = mat * glm::vec4(this->m_pos, 1);
        output.m_rotations.set_xyz(this->m_rotations.vec3() + ::decompose_rotations(mat));

        return output;
    }

    void EulerCamera::move_horizontal(glm::vec3 v) {
        v.y = 0;
        const auto rotated = make_rotation_y<glm::mat3>(this->m_rotations.y()) * v;

        this->m_pos.x += rotated.x;
        this->m_pos.z += rotated.z;
    }

    void EulerCamera::move_forward(const glm::vec3& v) {
        this->m_pos += ::make_rotation_yxz<glm::mat3>(this->m_rotations.vec3()) * v;
    }

}


// QuatCamera
namespace dal {

    glm::mat4 QuatCamera::make_view_mat() const {
        return glm::inverse(this->m_transform.make_mat4());
    }

    void QuatCamera::set_rot_xyz(const float x, const float y, const float z) {
        const auto rot_mat = ::make_rotation_yxz<glm::mat3>(x, y, z);
        this->m_transform.m_quat = glm::quat_cast(rot_mat);
    }

    void QuatCamera::add_rot_xyz(const float x, const float y, const float z) {
        auto& quat = this->m_transform.m_quat;

        {
            const auto quat_rotation = glm::mat3_cast(quat);
            const auto rotated_right = quat_rotation * ::DIRECTION_RIGHT;
            quat = glm::angleAxis(x, rotated_right) * quat;
        }

        {
            quat = glm::angleAxis(-y, ::GRAVITY_DIRECTION) * quat;
        }

        {
            const auto quat_rotation = glm::mat3_cast(quat);
            const auto rotated_front = quat_rotation * ::DIRECTION_FRONT;
            quat = glm::angleAxis(-z, rotated_front) * quat;
        }
    }

    void QuatCamera::rotate_head_up(const float delta_time) {
        auto& quat = this->m_transform.m_quat;

        const auto quat_rotation = glm::mat3_cast(quat);
        const auto rotated_up = quat_rotation * ::DIRECTION_UP;
        const auto rotated_right = quat_rotation * ::DIRECTION_RIGHT;

        const dal::Plane rot_plane{ glm::vec3{0}, glm::cross(rotated_right, ::GRAVITY_DIRECTION) };
        const auto rotated_up_on_plane = glm::normalize(rot_plane.find_closest_point(rotated_up));
        const auto angle_diff = glm::dot(rotated_up_on_plane, -::GRAVITY_DIRECTION);
        if (1.f == angle_diff)
            return;

        const auto angle_diff_radians = glm::acos(glm::clamp<float>(angle_diff, -1, 1));
        const auto axis_direction_sign = glm::dot(glm::cross(rotated_up_on_plane, -::GRAVITY_DIRECTION), rot_plane.normal()) > 0 ? 1 : -1;
        const auto delta_angle = glm::radians<float>(180) * delta_time;
        const auto actual_delta_angle = delta_angle >= angle_diff_radians ? angle_diff_radians : delta_angle;
        quat = glm::angleAxis(axis_direction_sign * actual_delta_angle, rot_plane.normal()) * quat;
    }

    QuatCamera QuatCamera::transform(const glm::mat4& mat) const {
        QuatCamera output;

        output.m_transform.m_pos = mat * glm::vec4{ this->m_transform.m_pos, 1 };
        output.m_transform.m_quat = glm::quat_cast(mat) * this->m_transform.m_quat;
        output.m_transform.m_scale = 1;

        return output;
    }

    // -z is forward, +x is right, y is discarded
    void QuatCamera::move_horizontal(glm::vec3 v) {

    }

    // -z is forward, +y is up, +x is right
    void QuatCamera::move_forward(const glm::vec3& v) {
        this->pos() += this->m_transform.make_mat3() * v;
    }

}


// Functions
namespace dal {

    glm::mat4 make_perspective_proj_mat(const float fov, const float ratio, const float near, const float far) {
        auto mat = glm::perspectiveRH_ZO<float>(fov, ratio, near, far);
        mat[1][1] *= -1;
        return mat;
    }

}
