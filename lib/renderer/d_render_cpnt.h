#pragma once

#include <glm/glm.hpp>


namespace dal {

    class ILight {

    public:
        glm::vec3 m_pos;
        glm::vec3 m_color;

    };


    class DLight : public ILight {

    private:
        glm::vec3 m_to_light_direc;

    public:
        void set_to_light_direc(const glm::vec3& v) {
            this->m_to_light_direc = glm::normalize(v);
        }

        void set_to_light_direc(const float x, const float y, const float z) {
            this->set_to_light_direc(glm::vec3{x, y, z});
        }

        auto& to_light_direc() const {
            return this->m_to_light_direc;
        }

    };


    class PLight : public ILight {

    };

}
