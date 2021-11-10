#pragma once

#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class EulerAnglesYXZ {

    private:
        glm::vec3 m_rotations{ 0, 0, 0 };

    public:
        const glm::vec3& vec3() const {
            return this->m_rotations;
        }

        auto x() const {
            return this->m_rotations.x;
        }

        auto y() const {
            return this->m_rotations.y;
        }

        auto z() const {
            return this->m_rotations.z;
        }

        void set_x(const float v);

        void set_y(const float v);

        void set_z(const float v);

        void set_xyz(const float x, const float y, const float z) {
            this->set_x(x);
            this->set_y(y);
            this->set_z(z);
        }

        void set_xyz(const glm::vec3& v) {
            this->set_x(v.x);
            this->set_y(v.y);
            this->set_z(v.z);
        }

        void add_x(const float v);

        void add_y(const float v);

        void add_z(const float v);

        void add_xyz(const float x, const float y, const float z) {
            this->add_x(x);
            this->add_y(y);
            this->add_z(z);
        }

        void add_xyz(const glm::vec3& v) {
            this->add_xyz(v.x, v.y, v.z);
        }

        glm::mat4 make_view_mat() const;

    };


    class Transform {

    public:
        glm::quat m_quat{1, 0, 0, 0};
        glm::vec3 m_pos{0, 0, 0};
        float m_scale = 1;

    public:
        glm::mat3 make_mat3() const;

        glm::mat4 make_mat4() const;

        void rotate(const float v, const glm::vec3& selector);

    };


    class ICamera {

    public:
        virtual ~ICamera() = default;

        virtual glm::vec3 view_pos() const = 0;

        virtual glm::mat4 make_view_mat() const = 0;

        std::array<glm::vec3, 8> make_frustum_vertices(const float fov, const float ratio, const float near, const float far) const;

    };


    class EulerCamera : public ICamera {

    private:
        EulerAnglesYXZ m_rotations;
        glm::vec3 m_pos{ 0, 0, 0 };

    public:
        glm::vec3& pos() {
            return this->m_pos;
        }

        glm::vec3 view_pos() const override {
            return this->m_pos;
        }

        glm::mat4 make_view_mat() const override;

        void set_rot_xyz(const float x, const float y, const float z);

        void add_rot_xyz(const float x, const float y, const float z);

        void add_rot_xyz(const glm::vec3& v) {
            this->add_rot_xyz(v.x, v.y, v.z);
        }

        void rotate_head_up(const float delta_time);

        EulerCamera transform(const glm::mat4& mat) const;

        // -z is forward, +x is right, y is discarded
        void move_horizontal(glm::vec3 v);

        // -z is forward, +y is up, +x is right
        void move_forward(const glm::vec3& v);

    };


    class QuatCamera : public ICamera {

    private:
        Transform m_transform;

        //----------------------------------------------------

    public:
        glm::vec3& pos() {
            return this->m_transform.m_pos;
        }

        glm::vec3 view_pos() const override {
            return this->m_transform.m_pos;
        }

        glm::mat4 make_view_mat() const override;

        void set_rot_xyz(const float x, const float y, const float z);

        void add_rot_xyz(const float x, const float y, const float z);

        void add_rot_xyz(const glm::vec3& v) {
            this->add_rot_xyz(v.x, v.y, v.z);
        }

        void rotate_head_up(const float delta_time);

        QuatCamera transform(const glm::mat4& mat) const;

        // -z is forward, +x is right, y is discarded
        void move_horizontal(glm::vec3 v);

        // -z is forward, +y is up, +x is right
        void move_forward(const glm::vec3& v);

    };


    glm::mat4 make_perspective_proj_mat(const float fov, const float ratio, const float near, const float far);

}
