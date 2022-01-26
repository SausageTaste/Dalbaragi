#include "d_engine.h"

#include <fmt/format.h>

#include <daltools/konst.h>

#include "d_logger.h"
#include "d_renderer_create.h"


#define DAL_TEST_FUNCTIONS false


namespace {

    const char* const MAIN_CONFIG_PATH = "_internal/config.json";


    auto make_move_direc(const dal::KeyInputManager& im) {
        glm::vec3 result{ 0, 0, 0 };

        const float move_factor = im.key_state_of(dal::KeyCode::lshfit).m_pressed ? 10.f : 1.f;

        if (im.key_state_of(dal::KeyCode::a).m_pressed) {
            result.x -= move_factor;
        }
        if (im.key_state_of(dal::KeyCode::d).m_pressed) {
            result.x += move_factor;
        }
        if (im.key_state_of(dal::KeyCode::w).m_pressed) {
            result.z -= move_factor;
        }
        if (im.key_state_of(dal::KeyCode::s).m_pressed) {
            result.z += move_factor;
        }

        if (im.key_state_of(dal::KeyCode::space).m_pressed) {
            result.y += move_factor;
        }
        if (im.key_state_of(dal::KeyCode::lctrl).m_pressed) {
            result.y -= move_factor;
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


#if DAL_TEST_FUNCTIONS

    void test(dal::Filesystem& filesys) {
        dalDebug("------------------------------------------------------");
        dalDebug("TEST");
        dalDebug("------------------------------------------------------");
        {
            auto file = filesys.open_write("_internal/hello/file/config2.json");
            dalAssert(file->is_ready());
            std::string test_data{ "Hello file!" };
            file->write(test_data.data(), test_data.size());
        }

        {
            const auto resolved_path = filesys.resolve("_internal/?/config2.json");
            dalAssert(resolved_path.has_value());
            dalDebug(resolved_path->make_str().c_str());

            auto file = filesys.open(*resolved_path);
            dalAssert(file->is_ready());
            dalDebug(file->read_stl<std::string>()->c_str());
        }

        {
            for (auto& x : filesys.list_files("_internal")) {
                dalDebug(x.c_str());
            }
        }

        dalDebug("------------------------------------------------------");
    }

#endif

}


namespace dal {

    LogChannel_FileOutput::LogChannel_FileOutput(dal::Filesystem& filesys)
        : m_filesys(filesys)
    {

    }

    LogChannel_FileOutput::~LogChannel_FileOutput() {

    }

    void LogChannel_FileOutput::put(
        const dal::LogLevel level, const char* const str,
        const int line, const char* const func, const char* const file
    ) {
        if (level < dal::LogLevel::error)
            return;

        std::unique_lock lck{ this->m_mut };

        this->m_buffer += fmt::format("[{}, {}, {}, {}] {}\n", dal::get_log_level_str(level), line, func, file, str);

        if (this->m_buffer.size() > this->FLUSH_SIZE) {
            this->flush();
        }
    }

    void LogChannel_FileOutput::flush() {
        if (this->m_buffer.empty())
            return;

        const auto file_path = fmt::format("_internal/log/log-{}.txt", dal::get_cur_sec());
        auto file = this->m_filesys.open_write(file_path);
        if (!file->is_ready()) {
            return;
        }

        file->write(this->m_buffer.data(), this->m_buffer.size());
        this->m_buffer.clear();
    }

}


namespace dal {

    bool EngineCreateInfo::check_validity() const {
        if (nullptr == this->m_filesystem)
            return false;

        return true;
    }

}


namespace dal {

    Engine::Engine(const EngineCreateInfo& create_info)
        : m_sign_mgr(crypto::CONTEXT_PARSER)
        , m_res_man(this->m_task_man, *create_info.m_filesystem, this->m_sign_mgr)
    {
        dalAssert(create_info.check_validity());
        this->m_create_info = create_info;

        this->m_task_man.init(2);
        this->m_renderer = dal::create_renderer_null();

        this->m_input_listeners.clear();
        this->m_input_listeners.push_back(&g_touch_dpad); g_touch_dpad.reset();
        this->m_input_listeners.push_back(&g_touch_view); g_touch_view.reset();

        // Load main config
        {
            auto file = create_info.m_filesystem->open(MAIN_CONFIG_PATH);
            if (file->is_ready()) {
                const auto buffer = file->read_stl<std::vector<uint8_t>>();
                if (buffer.has_value()) {
                    this->m_config.load_json(buffer->data(), buffer->size());
                }
            }
        }

        // Update renderer config
        {
            this->m_render_config.m_shader.m_atmos_dithering = this->m_config.m_renderer.m_atmos_dithering;
            this->m_render_config.m_shader.m_volumetric_atmos = this->m_config.m_renderer.m_volumetric_atmos;
        }

        this->m_lua.give_dependencies(this->m_scene, this->m_res_man);
        {
            auto file = this->m_create_info.m_filesystem->open(this->m_config.m_misc.m_startup_script_path);
            const auto content = file->read_stl<std::string>();
            dalAssertm(content.has_value(), fmt::format("Failed to find startup script file: {}", this->m_config.m_misc.m_startup_script_path).c_str());
            this->m_lua.exec(content->c_str());
            this->m_lua.call_void_func("on_engine_init");
        }

#if DAL_TEST_FUNCTIONS
        ::test(*this->m_create_info.m_filesystem);
#endif

        this->m_timer.check();
    }

    Engine::~Engine() {
        this->destory_vulkan();

        if (auto file = this->m_create_info.m_filesystem->open_write(::MAIN_CONFIG_PATH); file->is_ready()) {
            const auto data = this->m_config.export_str();
            file->write(data.data(), data.size());
        }

        this->m_lua.clear_dependencies();
        this->m_task_man.destroy();
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

        // Update camera
        {
            constexpr float MOVE_SPEED = 2;
            constexpr float ROT_SPEED = 1.5;

            const auto move_vec = (
                ::make_move_direc(this->input_manager().key_manager()) +
                ::make_move_direc(this->input_manager().gamepad_manager()) +
                g_touch_dpad.make_move_vec(this->m_screen_width, this->m_screen_height)
            );
            this->m_scene.m_euler_camera.move_forward(glm::vec3{move_vec.x, 0, move_vec.z} * delta_time_f * MOVE_SPEED);
            this->m_scene.m_euler_camera.pos().y += MOVE_SPEED * move_vec.y * delta_time_f;

            const auto rotation_delta = (
                ::make_rotation_angles(this->input_manager().key_manager()) * delta_time_f +
                ::make_rotation_angles(this->input_manager().gamepad_manager()) * delta_time_f +
                g_touch_view.check_view_vec(this->m_screen_width, this->m_screen_height) * 0.02f
            ) * ROT_SPEED;

            this->m_scene.m_euler_camera.add_rot_xyz(rotation_delta);

            // Rotate camera to make it's top look upward
            if (0.f == rotation_delta.z)
                this->m_scene.m_euler_camera.rotate_head_up(delta_time_f);
        }

        this->m_task_man.update();
        this->m_res_man.update();
        this->m_scene.update();

        this->m_lua.call_void_func("before_rendering_every_frame");

        this->m_renderer->update(this->m_scene.m_euler_camera, this->m_scene);
    }

    void Engine::init_vulkan(
        const unsigned win_width,
        const unsigned win_height,
        const surface_create_func_t surface_create_func,
        const std::vector<std::string>& extensions_str
    ) {
        this->m_screen_width = win_width;
        this->m_screen_height = win_height;

        if (0 == win_width || 0 == win_height) {
            this->m_res_man.invalidate_renderer();
            this->m_renderer = dal::create_renderer_null();
            return;
        }

        std::vector<const char*> extensions;
        for (const auto& x : extensions_str)
            extensions.push_back(x.c_str());

        this->m_renderer = dal::create_renderer_vulkan(
            this->m_create_info.m_window_title.c_str(),
            win_width,
            win_height,
            this->m_render_config,
            *this->m_create_info.m_filesystem,
            this->m_task_man,
            this->m_res_man,
            extensions,
            surface_create_func
        );

        this->m_res_man.set_renderer(*this->m_renderer.get());

        // This happens only once
        if (this->m_scene.m_registry.empty()) {
            this->m_lua.call_void_func("on_renderer_init");

            // Create a mirror
            {
                auto& mirror = this->m_scene.m_mirrors.emplace_back();

                mirror.m_mesh = this->m_res_man.request_mesh(std::make_unique<QuadMeshGenerator>(
                    glm::vec3{-2,  1, 0},
                    glm::vec3{-3, -1, 0},
                    glm::vec3{ 3, -1, 0},
                    glm::vec3{ 2,  1, 0}
                ));

                mirror.m_plane = Plane{
                    glm::vec3{-1,  1, 0},
                    glm::vec3{-1, -1, 0},
                    glm::vec3{ 1, -1, 0}
                };

                mirror.m_actor = this->m_res_man.request_actor();
                mirror.m_actor->m_transform.rotate(glm::radians<float>(90), glm::vec3{0, 1, 0});
                mirror.m_actor->m_transform.m_pos = glm::vec3{-14, 1.2, 0};
                mirror.m_actor->notify_transform_change();
            }

            // Create a portal pair
            {
                auto& portal_pair = this->m_scene.m_portal_pairs.emplace_back();

                {
                    auto& portal = portal_pair.m_portals[0];

                    portal.m_mesh = this->m_res_man.request_mesh(std::make_unique<QuadMeshGenerator>(
                        glm::vec3{-0.5,  1, 0},
                        glm::vec3{-0.5, -1, 0},
                        glm::vec3{ 0.5, -1, 0},
                        glm::vec3{ 0.5,  1, 0}
                    ));

                    portal.m_plane = PlaneOriented{
                        glm::vec3{ 0.5, -1, 0},
                        glm::vec3{ 0.5,  1, 0},
                        glm::vec3{-0.5,  1, 0}
                    };

                    portal.m_collider.emplace_back(Triangle{
                        glm::vec3{-0.5,  1, 0},
                        glm::vec3{-0.5, -1, 0},
                        glm::vec3{ 0.5, -1, 0}
                    });
                    portal.m_collider.emplace_back(Triangle{
                        glm::vec3{-0.5,  1, 0},
                        glm::vec3{ 0.5, -1, 0},
                        glm::vec3{ 0.5,  1, 0}
                    });

                    portal.m_actor = this->m_res_man.request_actor();
                }

                {
                    auto& portal = portal_pair.m_portals[1];

                    portal.m_mesh = portal_pair.m_portals[0].m_mesh;

                    portal.m_plane = portal_pair.m_portals[0].m_plane;

                    portal.m_collider = portal_pair.m_portals[0].m_collider;

                    portal.m_actor = this->m_res_man.request_actor();
                }
            }

            // Create a horizontal water plane
            {
                auto& water = this->m_scene.m_water_planes.emplace_back();

                water.m_mesh = this->m_res_man.request_mesh(std::make_unique<QuadMeshGenerator>(
                    glm::vec3{-5, 0, -5},
                    glm::vec3{-5, 0,  5},
                    glm::vec3{ 5, 0,  5},
                    glm::vec3{ 5, 0, -5}
                ));

                water.m_plane = Plane{
                    glm::vec3{-5, 0, -5},
                    glm::vec3{-5, 0,  5},
                    glm::vec3{ 5, 0,  5}
                };
            }
        }
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
