#include "d_engine.h"

#include "d_logger.h"


namespace {

    auto make_move_direc(const dal::KeyInputManager& im) {
        glm::vec3 result{ 0, 0, 0 };

        if (im.key_state_of(dal::KeyCode::a).m_pressed) {
            result.x -= 1;
        }
        if (im.key_state_of(dal::KeyCode::d).m_pressed) {
            result.x += 1;
        }
        if (im.key_state_of(dal::KeyCode::w).m_pressed) {
            result.z -= 1;
        }
        if (im.key_state_of(dal::KeyCode::s).m_pressed) {
            result.z += 1;
        }

        if (im.key_state_of(dal::KeyCode::space).m_pressed) {
            result.y += 1;
        }
        if (im.key_state_of(dal::KeyCode::lctrl).m_pressed) {
            result.y -= 1;
        }

        return result;
    }

    auto make_move_direc(const dal::GamepadInputManager& gm) {
        glm::vec3 result{ 0, 0, 0 };

        for (const auto& [id, state] : gm) {
            result.x += state.m_axis_left.x;
            result.z -= state.m_axis_left.y;
        }

        return result;
    }

    auto make_rotation_angles(const dal::KeyInputManager& im) {
        glm::vec2 result{ 0, 0 };

        if (im.key_state_of(dal::KeyCode::left).m_pressed) {
            result.y += 1;
        }
        if (im.key_state_of(dal::KeyCode::right).m_pressed) {
            result.y -= 1;
        }

        if (im.key_state_of(dal::KeyCode::up).m_pressed) {
            result.x += 1;
        }
        if (im.key_state_of(dal::KeyCode::down).m_pressed) {
            result.x -= 1;
        }

        return result;
    }

    auto make_rotation_angles(const dal::GamepadInputManager& m) {
        glm::vec2 result{ 0, 0 };

        for (const auto& [id, state] : m) {
            result.x += state.m_axis_right.y;
            result.y -= state.m_axis_right.x;
        }

        return result;
    }

}


namespace dal {

    bool EngineCreateInfo::check_validity() const {
        if (nullptr == this->m_window_title)
            return false;
        if (0 == this->m_init_width)
            return false;
        if (0 == this->m_init_height)
            return false;
        if (nullptr == this->m_filesystem)
            return false;
        if (!this->m_surface_create_func)
            return false;

        return true;
    }

}


namespace dal {

    Engine::Engine() {
        this->m_task_man.init(2);
    }

    Engine::Engine(const EngineCreateInfo& create_info) {
        this->m_task_man.init(2);
        this->init(create_info);
    }

    Engine::~Engine() {
        this->m_task_man.destroy();
        this->destroy();
    }

    void Engine::init(const EngineCreateInfo& create_info) {
        dalAssert(create_info.check_validity());

        this->destroy();

        this->m_vulkan_man.init(
            create_info.m_window_title,
            create_info.m_init_width,
            create_info.m_init_height,
            *create_info.m_filesystem,
            this->m_task_man,
            create_info.m_extensions,
            create_info.m_surface_create_func
        );

        this->m_camera.m_pos = {0, 2, 3};
        this->m_camera.m_rotations = {glm::radians<float>(-30), 0, 0};

        this->m_timer.check();
    }

    void Engine::destroy() {
        this->m_vulkan_man.destroy();
    }

    void Engine::update() {
        const float delta_time = this->m_timer.check_get_elapsed();

        // Process inputs
        {
            constexpr float MOVE_SPEED = 2;
            constexpr float ROT_SPEED = 1.5;

            const auto move_vec = ::make_move_direc(this->input_manager().key_manager()) + ::make_move_direc(this->input_manager().gamepad_manager());
            this->m_camera.move_forward(glm::vec3{move_vec.x, 0, move_vec.z} * delta_time * MOVE_SPEED);
            this->m_camera.m_pos.y += MOVE_SPEED * move_vec.y * delta_time;

            const auto rotation_angles = ::make_rotation_angles(this->input_manager().key_manager()) + ::make_rotation_angles(this->input_manager().gamepad_manager());
            this->m_camera.m_rotations.x += rotation_angles.x * ROT_SPEED * delta_time;
            this->m_camera.m_rotations.y += rotation_angles.y * ROT_SPEED * delta_time;

            if (this->input_manager().key_manager().key_state_of(dal::KeyCode::q).m_pressed) {
                this->m_camera.m_rotations.z += ROT_SPEED * delta_time;
            }
            if (this->input_manager().key_manager().key_state_of(dal::KeyCode::e).m_pressed) {
                this->m_camera.m_rotations.z -= ROT_SPEED * delta_time;
            }

            this->input_manager().touch_manager().queue().clear();
            this->input_manager().key_manager().queue().clear();
        }

        this->m_task_man.update();

        this->m_vulkan_man.update(this->m_camera);
    }

    void Engine::wait_device_idle() const {
        this->m_vulkan_man.wait_device_idle();
    }

    void Engine::on_screen_resize(const unsigned width, const unsigned height) {
        this->m_vulkan_man.on_screen_resize(width, height);
    }

}
