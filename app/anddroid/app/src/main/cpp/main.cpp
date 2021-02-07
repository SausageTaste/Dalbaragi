#include <jni.h>
#include <fmt/format.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <d_logger.h>
#include <d_vulkan_man.h>
#include <d_vulkan_header.h>


namespace {

    auto& logger = dal::LoggerSingleton::inst();

    class LogChannel_Logcat : public dal::ILogChannel {

    public:
        void put(
            const dal::LogLevel level, const char* const str,
            const int line, const char* const func, const char* const file
        ) override {
            __android_log_print(
                map_log_levels(level),
                "vulkan_practice",
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

        if (nullptr != state->userData) {
            auto& engine = *reinterpret_cast<dal::VulkanState*>(state->userData);

            engine.destroy();
            engine.init(
                "shit",
                ANativeWindow_getWidth(state->window),
                ANativeWindow_getHeight(state->window),
                instanceExt,
                surface_creator
            );
        }
        else {
            state->userData = new dal::VulkanState(
                "shit",
                ANativeWindow_getWidth(state->window),
                ANativeWindow_getHeight(state->window),
                instanceExt,
                surface_creator
            );
        }
    }

}


extern "C" {

    void handle_cmd(android_app* const state, const int32_t cmd) {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                dalInfo("handle cmd: init window");
                ::init(state);
                break;
            case APP_CMD_WINDOW_RESIZED:
                dalInfo("handle cmd: init window");
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
        logger.emplace_channel<LogChannel_Logcat>();

        int events;
        android_poll_source *pSource;
        do {
            if (ALooper_pollAll(0, nullptr, &events, (void **) &pSource) >= 0) {
                if (pSource) {
                    pSource->process(pApp, pSource);
                }
            }
        } while (!pApp->destroyRequested);
    }

}
