#include <jni.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <d_vulkan_man.h>
#include <d_vulkan_header.h>
#include <d_logger.h>


namespace {

    auto& logger = dal::LoggerSingleton::inst();


    void simple_print(const char* const text) {
        __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "%s", text);
    }

    void init(ANativeWindow* const native_window) {
        const std::vector<const char*> instanceExt{
            "VK_KHR_surface",
            "VK_KHR_android_surface",
        };
        const auto surface_creator = [native_window](void* vk_instance) -> void* {
            VkAndroidSurfaceCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = native_window,
            };

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            const auto create_result = vkCreateAndroidSurfaceKHR(
                reinterpret_cast<VkInstance>(vk_instance),
                &create_info,
                nullptr,
                &surface
            );

            if (VK_SUCCESS != create_result) {
                logger.simple_print("failed to create vulkan surface");
                return nullptr;
            }

            return reinterpret_cast<void*>(surface);
        };

        dal::VulkanState state;
        state.init("shit", instanceExt, surface_creator);
    }

}


extern "C" {

    void handle_cmd(android_app* const state, const int32_t cmd) {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: init window");
                ::init(state->window);
                break;
            case APP_CMD_WINDOW_RESIZED:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: init window");
                break;
            case APP_CMD_DESTROY:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: destroy");
                break;
            case APP_CMD_PAUSE:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: pause");
                break;
            case APP_CMD_RESUME:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: resume");
                break;
            case APP_CMD_START:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: start");
                break;
            case APP_CMD_STOP:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: stop");
                break;
            case APP_CMD_GAINED_FOCUS:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: focus gained");
                break;
            case APP_CMD_LOST_FOCUS:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: focus lost");
                break;
            case APP_CMD_INPUT_CHANGED:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: input changed");
                break;
            case APP_CMD_TERM_WINDOW:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: window terminated");
                break;
            case APP_CMD_SAVE_STATE:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd: save state");
                break;
            default:
                __android_log_print(ANDROID_LOG_VERBOSE, "vulkan_practice", "handle cmd; unknown (%d)", cmd);
                break;
        }
    }

    void android_main(struct android_app *pApp) {
        pApp->onAppCmd = handle_cmd;
        dal::LoggerSingleton::inst().add_print_func(::simple_print);

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
