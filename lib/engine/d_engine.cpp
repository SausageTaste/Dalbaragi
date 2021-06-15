#include "d_engine.h"

#include <fmt/format.h>

#include "d_logger.h"
#include "d_renderer_create.h"


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

        for (const auto& [id, state] : gm.pad_list()) {
            result.x += state.m_axis_left.x;
            result.z -= state.m_axis_left.y;
        }

        return result;
    }

    glm::vec3 make_rotation_angles(const dal::KeyInputManager& im) {
        glm::vec3 result{0};

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

        if (im.key_state_of(dal::KeyCode::q).m_pressed)
             result.z += 1;
        if (im.key_state_of(dal::KeyCode::e).m_pressed)
             result.z -= 1;

        return result;
    }

    glm::vec3 make_rotation_angles(const dal::GamepadInputManager& m) {
        glm::vec3 result{0};

        for (const auto& [id, state] : m.pad_list()) {
            result.x += state.m_axis_right.y;
            result.y -= state.m_axis_right.x;
        }

        return result;
    }


    class TouchDpad : public dal::IInputListener {

    private:
        dal::touchID_t m_controlling_id = dal::NULL_TOUCH_ID;
        glm::vec2 m_last_pos, m_down_pos;

    public:
        void reset() {
            this->m_controlling_id = dal::NULL_TOUCH_ID;
        }

        dal::InputConsumeResult on_touch_event(const dal::TouchEvent& e) override {
            if (dal::NULL_TOUCH_ID == this->m_controlling_id) {
                switch (e.m_action_type) {
                    case dal::TouchActionType::down:
                        this->m_controlling_id = e.m_id;
                        this->m_down_pos = e.m_pos;
                        this->m_last_pos = e.m_pos;
                        return dal::InputConsumeResult::owned;
                    default:
                        return dal::InputConsumeResult::ignored;
                }
            }
            else {
                if (e.m_id != this->m_controlling_id)
                    return dal::InputConsumeResult::ignored;

                switch (e.m_action_type) {
                    case dal::TouchActionType::down:
                    case dal::TouchActionType::move:
                        this->m_last_pos = e.m_pos;
                        return dal::InputConsumeResult::owned;
                    case dal::TouchActionType::up:
                        this->m_controlling_id = dal::NULL_TOUCH_ID;
                        return dal::InputConsumeResult::done_with;
                    default:
                        return dal::InputConsumeResult::owned;
                }
            }
        }

        bool is_in_bound(const float x, const float y, const float w, const float h) override {
            return x < w * 0.4;
        };

        glm::vec3 make_move_vec(const float w, const float h) const {
            if (dal::NULL_TOUCH_ID == this->m_controlling_id)
                return glm::vec3{0};

            const auto circle_radius = std::min(w, h) / 6.f;
            const auto a = (this->m_last_pos - this->m_down_pos) / circle_radius;
            const float len_spr = glm::dot(a, a);
            if (0.001f > len_spr) {
                return glm::vec3{0};
            }
            else if (1.f < len_spr) {
                const auto b = a / static_cast<float>(sqrt(len_spr));
                return glm::vec3{b.x, 0, b.y};
            }
            else {
                return glm::vec3{a.x, 0, a.y};
            }
        }

    } g_touch_dpad;


    class TouchViewController : public dal::IInputListener {

    private:
        dal::touchID_t m_controlling_id = dal::NULL_TOUCH_ID;
        glm::vec2 m_last_pos, m_accum_pos;

    public:
        void reset() {
            this->m_controlling_id = dal::NULL_TOUCH_ID;
        }

        dal::InputConsumeResult on_touch_event(const dal::TouchEvent& e) override {
            if (dal::NULL_TOUCH_ID == this->m_controlling_id) {
                switch (e.m_action_type) {
                    case dal::TouchActionType::down:
                        //dalInfo(fmt::format("Touch down in view: {}", e.m_id).c_str());
                        this->m_controlling_id = e.m_id;
                        this->m_accum_pos = glm::vec2{0};
                        this->m_last_pos = e.m_pos;
                        return dal::InputConsumeResult::owned;
                    default:
                        return dal::InputConsumeResult::ignored;
                }
            }
            else {
                if (e.m_id != this->m_controlling_id)
                    return dal::InputConsumeResult::ignored;

                switch (e.m_action_type) {
                    case dal::TouchActionType::down:
                    case dal::TouchActionType::move:
                        //dalInfo(fmt::format("Touch move in view: {}", e.m_id).c_str());
                        this->m_accum_pos += e.m_pos - this->m_last_pos;
                        this->m_last_pos = e.m_pos;
                        return dal::InputConsumeResult::owned;
                    case dal::TouchActionType::up:
                        //dalInfo(fmt::format("Touch up in view: {}", e.m_id).c_str());
                        this->m_controlling_id = dal::NULL_TOUCH_ID;
                        return dal::InputConsumeResult::done_with;
                    default:
                        return dal::InputConsumeResult::owned;
                }
            }
        }

        bool is_in_bound(const float x, const float y, const float w, const float h) override {
            return true;
        };

        glm::vec3 check_view_vec(const float w, const float h) {
            if (dal::NULL_TOUCH_ID == this->m_controlling_id)
                return glm::vec3{0};

            const auto accum = glm::vec3{ -this->m_accum_pos.y, -this->m_accum_pos.x, 0 } / std::min(w, h) * 200.f;
            this->m_accum_pos = glm::vec3{0};
            return accum;
        }

    } g_touch_view;

}


namespace {

    void populate_models(dal::Scene& scene, dal::ResourceManager& res_man) {
        // Honoka
        {
            const auto entity = scene.m_registry.create();
            auto model = res_man.request_model("_asset/model/honoka_basic_3.dmd");

            auto& cpnt_model = scene.m_registry.emplace<dal::cpnt::Model>(entity);
            cpnt_model.m_model = model;

            auto& cpnt_actor = scene.m_registry.emplace<dal::cpnt::Actor>(entity);

            cpnt_actor.m_actors.push_back(res_man.request_actor());
            cpnt_actor.m_actors.back()->m_transform.m_scale = 0.3;
            cpnt_actor.m_actors.back()->apply_changes();

            cpnt_actor.m_actors.push_back(res_man.request_actor());
            cpnt_actor.m_actors.back()->m_transform.m_pos = glm::vec3{ -2, 0, 0 };
            cpnt_actor.m_actors.back()->m_transform.rotate(glm::radians<float>(90), glm::vec3{0, 1, 0});
            cpnt_actor.m_actors.back()->m_transform.m_scale = 0.3;
            cpnt_actor.m_actors.back()->apply_changes();
        }

        // Character Running
        {
            const auto entity = scene.m_registry.create();
            auto model = res_man.request_model_skinned("_asset/model/Character Running.dmd");

            auto& cpnt_model = scene.m_registry.emplace<dal::cpnt::ModelSkinned>(entity);
            cpnt_model.m_model = model;

            auto& cpnt_actor = scene.m_registry.emplace<dal::cpnt::ActorAnimated>(entity);

            cpnt_actor.m_actors.push_back(res_man.request_actor_skinned());
            cpnt_actor.m_actors.back()->m_transform.m_pos = glm::vec3{ 2, 0, 0 };
            cpnt_actor.m_actors.back()->m_transform.rotate(glm::radians<float>(90), glm::vec3{0, 1, 0});
            cpnt_actor.m_actors.back()->m_transform.m_scale = 0.15;
            cpnt_actor.m_actors.back()->apply_transform(dal::FrameInFlightIndex{0});

            cpnt_actor.m_actors.back()->m_anim_state.setSelectedAnimeIndex(1);
        }

        // Sponza
        {
            const auto entity = scene.m_registry.create();
            auto model = res_man.request_model("_asset/model/sponza.dmd");

            auto& cpnt_model = scene.m_registry.emplace<dal::cpnt::Model>(entity);
            cpnt_model.m_model = model;

            auto& cpnt_actor = scene.m_registry.emplace<dal::cpnt::Actor>(entity);

            cpnt_actor.m_actors.push_back(res_man.request_actor());
            cpnt_actor.m_actors.back()->m_transform.m_scale = 0.01;
            cpnt_actor.m_actors.back()->m_transform.rotate(glm::radians<float>(90), glm::vec3{1, 0, 0});
            cpnt_actor.m_actors.back()->apply_changes();
        }

        dalInfo("Scene populated");
    }

}


namespace dal {

    bool EngineCreateInfo::check_validity() const {
        if (nullptr == this->m_filesystem)
            return false;
        if (!this->m_surface_create_func)
            return false;

        return true;
    }

}


namespace dal {

    Engine::Engine(const EngineCreateInfo& create_info)
        : m_res_man(this->m_task_man, *create_info.m_filesystem)
    {
        this->m_task_man.init(2);
        this->init(create_info);
    }

    Engine::~Engine() {
        this->m_task_man.destroy();
        this->destroy();
    }

    void Engine::init(const EngineCreateInfo& create_info) {
        this->destroy();
        this->m_renderer = dal::create_renderer_null();

        dalAssert(create_info.check_validity());
        this->m_create_info = create_info;

        this->m_input_listeners.clear();
        this->m_input_listeners.push_back(&g_touch_dpad); g_touch_dpad.reset();
        this->m_input_listeners.push_back(&g_touch_view); g_touch_view.reset();

        this->m_timer.check();
    }

    void Engine::destroy() {
        this->destory_vulkan();
    }

    void Engine::update() {
        const auto delta_time = this->m_timer.check_get_elapsed();
        const auto delta_time_f = static_cast<float>(delta_time);

        // Process inputs
        {
            for (const auto& e : this->input_manager().touch_manager()) {
                this->m_input_dispatch.dispatch(
                    this->m_input_listeners.data(),
                    this->m_input_listeners.data() + this->m_input_listeners.size(),
                    e, this->m_screen_width, this->m_screen_height
                );
            }

            this->input_manager().touch_manager().queue().clear();
            this->input_manager().key_manager().queue().clear();
        }
        {
            constexpr float MOVE_SPEED = 2;
            constexpr float ROT_SPEED = 1.5;

            const auto move_vec = (
                ::make_move_direc(this->input_manager().key_manager()) +
                ::make_move_direc(this->input_manager().gamepad_manager()) +
                g_touch_dpad.make_move_vec(this->m_screen_width, this->m_screen_height)
            );
            this->m_scene.m_euler_camera.move_forward(glm::vec3{move_vec.x, 0, move_vec.z} * delta_time_f * MOVE_SPEED);
            this->m_scene.m_euler_camera.m_pos.y += MOVE_SPEED * move_vec.y * delta_time_f;

            this->m_scene.m_euler_camera.m_rotations += (
                ::make_rotation_angles(this->input_manager().key_manager()) * delta_time_f +
                ::make_rotation_angles(this->input_manager().gamepad_manager()) * delta_time_f +
                g_touch_view.check_view_vec(this->m_screen_width, this->m_screen_height) * 0.02f
            ) * ROT_SPEED;
        }

        this->m_task_man.update();
        this->m_res_man.update();
        this->m_scene.update();

        this->m_renderer->update(this->m_scene.m_euler_camera, this->m_scene.make_render_list());
    }

    void Engine::init_vulkan(const unsigned win_width, const unsigned win_height) {
        this->m_screen_width = win_width;
        this->m_screen_height = win_height;

        std::vector<const char*> extensions;
        for (const auto& x : this->m_create_info.m_extensions) {
            extensions.push_back(x.c_str());
        }

        this->m_renderer = dal::create_renderer_vulkan(
            this->m_create_info.m_window_title.c_str(),
            win_width,
            win_height,
            *this->m_create_info.m_filesystem,
            this->m_task_man,
            this->m_res_man,
            extensions,
            this->m_create_info.m_surface_create_func
        );

        this->m_res_man.set_renderer(*this->m_renderer.get());

        if (this->m_scene.m_registry.empty())
            ::populate_models(this->m_scene, this->m_res_man);
    }

    void Engine::destory_vulkan() {
        this->m_res_man.invalidate_renderer();
        this->m_renderer = dal::create_renderer_null();
    }

    void Engine::wait_device_idle() const {
        this->m_renderer->wait_idle();
    }

    void Engine::on_screen_resize(const unsigned width, const unsigned height) {
        this->m_screen_width = width;
        this->m_screen_height = height;
        this->m_renderer->on_screen_resize(width, height);
    }

}
