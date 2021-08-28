#include <unordered_map>

#include <jni.h>
#include <fmt/format.h>
#include <android/log.h>

#include <d_logger.h>
#include <d_vulkan_header.h>
#include <d_engine.h>
#include <d_timer.h>

#include "filesys_android.h"


namespace {

    dal::Filesystem g_filesys;

    dal::surface_create_func_t g_surface_create_func;

    const std::vector<std::string> INSTANCE_EXTENSIONS{
        "VK_KHR_surface",
        "VK_KHR_android_surface",
    };


    class LogChannel_Logcat : public dal::ILogChannel {

    public:
        void put(
            const dal::LogLevel level, const char* const str,
            const int line, const char* const func, const char* const file
        ) override {
            __android_log_print(
                map_log_levels(level),
                "dalbaragi",
                "[%s] %s",
                dal::get_log_level_str(level),
                str
            );

            if (static_cast<int>(level) >= static_cast<int>(dal::LogLevel::warning))
                return;
            else
                return;
        }

    private:
        static android_LogPriority map_log_levels(const dal::LogLevel level) {
            switch (level) {
                case dal::LogLevel::verbose:
                    return ANDROID_LOG_VERBOSE;
                case dal::LogLevel::info:
                    return ANDROID_LOG_INFO;
                case dal::LogLevel::debug:
                    return ANDROID_LOG_DEBUG;
                case dal::LogLevel::warning:
                    return ANDROID_LOG_WARN;
                case dal::LogLevel::error:
                    return ANDROID_LOG_ERROR;
                case dal::LogLevel::fatal:
                    return ANDROID_LOG_FATAL;
                default:
                    return ANDROID_LOG_UNKNOWN;
            }
        }

    };


    void init(android_app* const state) {
        g_filesys.init(
            std::make_unique<dal::AssetManagerAndroid>(state->activity->assetManager),
            std::make_unique<dal::UserDataManagerAndroid>(),
            std::make_unique<dal::InternalManagerAndroid>(state->activity->internalDataPath)
        );

        g_surface_create_func = [state](void* vk_instance) -> uint64_t {
            VkAndroidSurfaceCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = state->window,
            };

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            const auto create_result = vkCreateAndroidSurfaceKHR(
                reinterpret_cast<VkInstance>(vk_instance),
                &create_info,
                nullptr,
                &surface
            );
            dalAssert(VK_SUCCESS == create_result);

#ifdef DAL_SYS_X32
            return surface;
#elif defined(DAL_SYS_X64)
            return reinterpret_cast<uint64_t>(surface);
#else
            #error Not supported system bit size
#endif
        };

        dal::EngineCreateInfo engine_info;
        engine_info.m_window_title = "Dalbrargi Android";
        engine_info.m_filesystem = &g_filesys;

        dalAssert(nullptr == state->userData);
        state->userData = new dal::Engine(engine_info);
    }

    void on_content_rect_changed(android_app* const state) {
        if (nullptr == state->userData)
            return;

        auto& engine = *reinterpret_cast<dal::Engine*>(state->userData);
        const auto width  = state->contentRect.right  - state->contentRect.left;
        const auto height = state->contentRect.bottom - state->contentRect.top;

        engine.on_screen_resize(width, height);
    }

}


namespace {

    enum class InputSource {
        unknown,
        touch_screen,
        keyboard,
        joystick,
        game_pad_btn,
        touch_pad_mouse,
        touch_pen,
    };

    enum class EventType {
        unknown,
        motion,
        key,
    };


    struct EventInfo {
        InputSource m_source = InputSource::unknown;
        EventType m_type = EventType::unknown;
    };

    auto interpret_event(AInputEvent* const event) {
        ::EventInfo output;

        // Input source
        {
            const auto input_src_bits = static_cast<unsigned>(AInputEvent_getSource(event));

            if (AINPUT_SOURCE_KEYBOARD == input_src_bits)
                output.m_source = ::InputSource::keyboard;
            else if (AINPUT_SOURCE_TOUCHSCREEN == input_src_bits)
                output.m_source = ::InputSource::touch_screen;
            else if (AINPUT_SOURCE_JOYSTICK == input_src_bits)
                output.m_source = ::InputSource::joystick;
            else if ((AINPUT_SOURCE_KEYBOARD | AINPUT_SOURCE_GAMEPAD) == input_src_bits)
                output.m_source = ::InputSource::game_pad_btn;
            else if ((AINPUT_SOURCE_TOUCHSCREEN | AINPUT_SOURCE_MOUSE) == input_src_bits)
                output.m_source = ::InputSource::touch_pad_mouse;
            else if ((AINPUT_SOURCE_TOUCHSCREEN | AINPUT_SOURCE_STYLUS) == input_src_bits)
                output.m_source = ::InputSource::touch_pen;
            else
                dalWarn(fmt::format("unknown source {:x}", input_src_bits).c_str());
        }

        // Type
        {
            const auto event_type_bits = AInputEvent_getType(event);

            if (AINPUT_EVENT_TYPE_MOTION == event_type_bits)
                output.m_type = ::EventType::motion;
            else if (AINPUT_EVENT_TYPE_KEY == event_type_bits)
                output.m_type = ::EventType::key;
            else
                dalWarn(fmt::format("unknown event type {:x}", event_type_bits).c_str());
        }

        return output;
    }

}


namespace {

    int parse_pointer_index(AInputEvent* const event) {
        return (AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    }

    dal::TouchActionType determine_motion_action_type(AInputEvent* const event) {
        const auto type = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

        switch (type) {
            case AMOTION_EVENT_ACTION_CANCEL:
                return dal::TouchActionType::cancel;

            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                return dal::TouchActionType::down;
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                return dal::TouchActionType::up;
            case AMOTION_EVENT_ACTION_MOVE:
                return dal::TouchActionType::move;

            case AMOTION_EVENT_ACTION_HOVER_ENTER:
                return dal::TouchActionType::hover_enter;
            case AMOTION_EVENT_ACTION_HOVER_MOVE:
                return dal::TouchActionType::hover_move;
            case AMOTION_EVENT_ACTION_HOVER_EXIT:
                return dal::TouchActionType::hover_exit;

            default:
                dalWarn(fmt::format("Unknown AMOTION_EVENT_ACTION: {}", type).c_str());
                return dal::TouchActionType::cancel;
        }
    }

    void handle_motion_event(AInputEvent* const event, dal::TouchInputManager& tm) {
        const auto num_pointers = AMotionEvent_getPointerCount(event);
        const auto action_type = ::determine_motion_action_type(event);

        if (dal::TouchActionType::move == action_type) {
            for (int i = 0; i < num_pointers; i++) {
                const auto pointer_id = AMotionEvent_getPointerId(event, i);
                const auto raw_x = AMotionEvent_getRawX(event, i);
                const auto raw_y = AMotionEvent_getRawY(event, i);

                dal::TouchEvent te;
                te.m_id = pointer_id;
                te.m_action_type = action_type;
                te.m_time_sec = dal::get_cur_sec();
                te.m_pos = glm::vec2{ raw_x, raw_y };

                tm.push_back(te);
            }
        }
        else {
            const auto pointer_index = ::parse_pointer_index(event);
            const auto pointer_id = AMotionEvent_getPointerId(event, pointer_index);
            const auto raw_x = AMotionEvent_getRawX(event, pointer_index);
            const auto raw_y = AMotionEvent_getRawY(event, pointer_index);

            dal::TouchEvent te;
            te.m_id = pointer_id;
            te.m_action_type = action_type;
            te.m_time_sec = dal::get_cur_sec();
            te.m_pos = glm::vec2{ raw_x, raw_y };

            tm.push_back(te);
        }
    }


    dal::KeyCode map_android_key_code(const int key_code) {
        if (AKEYCODE_A <= key_code && key_code <= AKEYCODE_Z) {
            auto index = key_code - AKEYCODE_A + static_cast<int>(dal::KeyCode::a);
            return static_cast<dal::KeyCode>(index);
        }
        else if (AKEYCODE_0 <= key_code && key_code <= AKEYCODE_9) {
            auto index = key_code - AKEYCODE_0 + static_cast<int>(dal::KeyCode::n0);
            return static_cast<dal::KeyCode>(index);
        }
        else {
            static const std::unordered_map<int, dal::KeyCode> map{
                {AKEYCODE_GRAVE,  dal::KeyCode::backquote},
                {AKEYCODE_MINUS,         dal::KeyCode::minus},
                {AKEYCODE_EQUALS,         dal::KeyCode::equal},
                {AKEYCODE_LEFT_BRACKET,  dal::KeyCode::lbracket},
                {AKEYCODE_RIGHT_BRACKET, dal::KeyCode::rbracket},
                {AKEYCODE_BACKSLASH,     dal::KeyCode::backslash},
                {AKEYCODE_SEMICOLON,     dal::KeyCode::semicolon},
                {AKEYCODE_APOSTROPHE,    dal::KeyCode::quote},
                {AKEYCODE_COMMA,         dal::KeyCode::comma},
                {AKEYCODE_PERIOD,        dal::KeyCode::period},
                {AKEYCODE_SLASH,         dal::KeyCode::slash},

                {AKEYCODE_SPACE,         dal::KeyCode::space},
                {AKEYCODE_ENTER,         dal::KeyCode::enter},
                {AKEYCODE_DEL,     dal::KeyCode::backspace},
                {AKEYCODE_TAB,           dal::KeyCode::tab},

                {AKEYCODE_ESCAPE,        dal::KeyCode::escape},
                {AKEYCODE_SHIFT_LEFT,    dal::KeyCode::lshfit},
                {AKEYCODE_SHIFT_RIGHT,   dal::KeyCode::rshfit},
                {AKEYCODE_CTRL_LEFT,  dal::KeyCode::lctrl},
                {AKEYCODE_CTRL_RIGHT, dal::KeyCode::rctrl},
                {AKEYCODE_ALT_LEFT,      dal::KeyCode::lalt},
                {AKEYCODE_ALT_RIGHT,     dal::KeyCode::ralt},
                {AKEYCODE_DPAD_UP,            dal::KeyCode::up},
                {AKEYCODE_DPAD_DOWN,          dal::KeyCode::down},
                {AKEYCODE_DPAD_LEFT,          dal::KeyCode::left},
                {AKEYCODE_DPAD_RIGHT,         dal::KeyCode::right},
            };

            const auto res = map.find(key_code);
            if (res == map.end()) {
                return dal::KeyCode::unknown;
            }
            else {
                return res->second;
            }
        }
    }

    void handle_keyboard_event(AInputEvent* const event, dal::KeyInputManager& km) {
        const auto action = AKeyEvent_getAction(event);
        const auto key_code = AKeyEvent_getKeyCode(event);
        const auto meta_state = AKeyEvent_getMetaState(event);

        dal::KeyEvent e;
        e.m_time_sec = dal::get_cur_sec();
        e.m_key = ::map_android_key_code(key_code);

        if (dal::KeyCode::unknown == e.m_key) {
            dalWarn(fmt::format("Unknown android key code: {}", key_code).c_str())
        }

        e.reset_modifier_states();
        if (AMETA_SHIFT_ON & meta_state)
            e.set_modifier_state(dal::KeyModifier::shift, true);
        if (AMETA_CTRL_ON & meta_state)
            e.set_modifier_state(dal::KeyModifier::ctrl, true);
        if (AMETA_ALT_ON & meta_state)
            e.set_modifier_state(dal::KeyModifier::alt, true);
        if (AMETA_CAPS_LOCK_ON & meta_state)
            e.set_modifier_state(dal::KeyModifier::caps_lock, true);
        if (AMETA_NUM_LOCK_ON & meta_state)
            e.set_modifier_state(dal::KeyModifier::num_lock, true);

        switch (action) {
            case AKEY_EVENT_ACTION_DOWN:
                e.m_action_type = km.key_state_of(e.m_key).m_pressed ? e.m_action_type = dal::KeyActionType::repeat : dal::KeyActionType::down;
                break;
            case AKEY_EVENT_ACTION_UP:
                e.m_action_type = dal::KeyActionType::up;
                break;
            default:
                dalWarn(fmt::format("Unknown android key action: {}", action).c_str());
                break;
        }

        km.push_back(e);
    }


    void handle_joystick_event(AInputEvent* const event, const EventInfo& event_info, dal::GamepadInputManager& gm) {
        const auto pointer_index = ::parse_pointer_index(event);

        auto& game_pad = gm.get_gamepad_state(0);

        if (::EventType::motion == event_info.m_type) {
            game_pad.m_dpad.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X, pointer_index);
            game_pad.m_dpad.y = -AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_Y, pointer_index);

            game_pad.m_axis_left.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, pointer_index);
            game_pad.m_axis_left.y = -AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, pointer_index);

            game_pad.m_axis_right.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, pointer_index);
            game_pad.m_axis_right.y = -AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, pointer_index);

            game_pad.m_trigger_left = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_BRAKE, pointer_index);
            game_pad.m_trigger_right = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_GAS, pointer_index);
        }
    }

}


extern "C" {

    int32_t handle_input(struct android_app* const state, AInputEvent* const event) {
        if (nullptr == state->userData)
            return 0;

        auto &engine = *reinterpret_cast<dal::Engine*>(state->userData);
        const auto event_info = ::interpret_event(event);

        if (::InputSource::touch_screen == event_info.m_source) {
            ::handle_motion_event(event, engine.input_manager().touch_manager());
        }
        else if (::InputSource::keyboard == event_info.m_source) {
            ::handle_keyboard_event(event, engine.input_manager().key_manager());
        }
        else if (::InputSource::joystick == event_info.m_source) {
            ::handle_joystick_event(event, event_info, engine.input_manager().gamepad_manager());
        }
        else {
            dalWarn(fmt::format(
                "Not processed a input: type={}, source={}",
                static_cast<int>(event_info.m_type),
                static_cast<int>(event_info.m_source)
            ).c_str());
        }

        return 0;
    }

    void handle_cmd(android_app* const state, const int32_t cmd) {
        dalAssert(nullptr != state->userData);
        auto& engine = *reinterpret_cast<dal::Engine*>(state->userData);

        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                dalInfo("handle cmd: init window");
                engine.init_vulkan(
                    ANativeWindow_getWidth(state->window),
                    ANativeWindow_getHeight(state->window),
                    g_surface_create_func,
                    INSTANCE_EXTENSIONS
                );
                break;
            case APP_CMD_TERM_WINDOW:
                dalInfo("handle cmd: window terminated");
                engine.destory_vulkan();
                break;
            case APP_CMD_CONTENT_RECT_CHANGED:
                dalInfo("handle cmd: content rect changed");
                ::on_content_rect_changed(state);
                break;
            case APP_CMD_CONFIG_CHANGED:
                dalInfo("handle cmd: config changed");
                break;
            case APP_CMD_WINDOW_RESIZED:
                dalInfo("handle cmd: window resized");
                break;
            case APP_CMD_WINDOW_REDRAW_NEEDED:
                dalInfo("handle cmd: window redraw needed");
                break;
            case APP_CMD_DESTROY:
                dalInfo("handle cmd: destroy");
                break;
            case APP_CMD_PAUSE:
                dalInfo("handle cmd: pause");
                break;
            case APP_CMD_RESUME:
                dalInfo("handle cmd: resume");
                break;
            case APP_CMD_START:
                dalInfo("handle cmd: start");
                break;
            case APP_CMD_STOP:
                dalInfo("handle cmd: stop");
                break;
            case APP_CMD_GAINED_FOCUS:
                dalInfo("handle cmd: focus gained");
                break;
            case APP_CMD_LOST_FOCUS:
                dalInfo("handle cmd: focus lost");
                break;
            case APP_CMD_INPUT_CHANGED:
                dalInfo("handle cmd: input changed");
                break;
            case APP_CMD_SAVE_STATE:
                dalInfo("handle cmd: save state");
                break;
            default:
                dalInfo( fmt::format("handle cmd; unknown ({})", cmd).c_str() );
                break;
        }
    }

    void android_main(struct android_app *pApp) {
        pApp->onAppCmd = handle_cmd;
        pApp->onInputEvent = handle_input;
        dal::LoggerSingleton::inst().emplace_channel<LogChannel_Logcat>();

        ::init(pApp);

        int events;
        android_poll_source *pSource;
        do {
            if (ALooper_pollAll(0, nullptr, &events, (void **) &pSource) >= 0) {
                if (pSource) {
                    pSource->process(pApp, pSource);
                }
            }

            if (nullptr != pApp->userData) {
                auto &engine = *reinterpret_cast<dal::Engine*>(pApp->userData);
                engine.update();
            }
        } while (!pApp->destroyRequested);

        if (nullptr != pApp->userData) {
            auto &engine = *reinterpret_cast<dal::Engine*>(pApp->userData);
            engine.wait_device_idle();
        }
    }

}
