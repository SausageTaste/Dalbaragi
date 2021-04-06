#include <jni.h>
#include <fmt/format.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <d_logger.h>
#include <d_vulkan_header.h>
#include <d_engine.h>
#include <d_timer.h>


namespace {

    dal::filesystem::AssetManager g_asset_manager;


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
        g_asset_manager.set_android_asset_manager(state->activity->assetManager);

        const std::vector<const char*> instanceExt{
            "VK_KHR_surface",
            "VK_KHR_android_surface",
        };
        const auto surface_creator = [state](void* vk_instance) -> void* {
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

            return reinterpret_cast<void*>(surface);
        };

        dal::EngineCreateInfo engine_info;
        engine_info.m_window_title = "Dalbrargi Android";
        engine_info.m_init_width = ANativeWindow_getWidth(state->window);
        engine_info.m_init_height = ANativeWindow_getHeight(state->window);
        engine_info.m_asset_mgr = &g_asset_manager;
        engine_info.m_extensions = instanceExt;
        engine_info.m_surface_create_func = surface_creator;

        if (nullptr != state->userData) {
            auto& engine = *reinterpret_cast<dal::Engine*>(state->userData);
            engine.init(engine_info);
        }
        else {
            state->userData = new dal::Engine(engine_info);
        }
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

    auto parse_pointer_index(AInputEvent* const event) {
        return (AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    }

    auto determine_motion_action_type(AInputEvent* const event) {
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
        const auto pointer_index = ::parse_pointer_index(event);
        const auto pointer_id = AMotionEvent_getPointerId(event, pointer_index);
        const auto raw_x = AMotionEvent_getRawX(event, pointer_index);
        const auto raw_y = AMotionEvent_getRawY(event, pointer_index);
        const auto action_type = ::determine_motion_action_type(event);

        dal::TouchEvent te;
        te.m_id = pointer_id;
        te.m_action_type = action_type;
        te.m_time_sec = dal::get_cur_sec();
        te.m_pos = glm::vec2{ raw_x, raw_y };

        tm.push_back(te);
    }

}


extern "C" {

    int32_t handle_input(struct android_app* const state, AInputEvent* const event) {
        if (nullptr == state->userData)
            return 0;

        auto &engine = *reinterpret_cast<dal::Engine*>(state->userData);
        const auto event_type = AInputEvent_getType(event);

        if (AINPUT_EVENT_TYPE_MOTION == event_type) {
            ::handle_motion_event(event, engine.input_manager().touch_manager());
        }
        else if (AINPUT_EVENT_TYPE_KEY == event_type) {
            dalInfo(fmt::format(
                "Key event: action={}, key code={}, meta state={}",
                AKeyEvent_getAction(event),
                AKeyEvent_getKeyCode(event),
                AKeyEvent_getMetaState(event)
            ).c_str());
        }

        return 0;
    }

    void handle_cmd(android_app* const state, const int32_t cmd) {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                dalInfo("handle cmd: init window");
                ::init(state);
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
            case APP_CMD_TERM_WINDOW:
                dalInfo("handle cmd: window terminated");
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
