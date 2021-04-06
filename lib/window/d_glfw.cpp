#include "d_glfw.h"

#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "d_timer.h"


namespace {

    // The result might be nullptr
    GLFWwindow* create_glfw_window(const unsigned width, const unsigned height, const char* const title) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        return glfwCreateWindow(width, height, title, nullptr, nullptr);
    }

    VkSurfaceKHR create_vk_surface(const VkInstance instance, GLFWwindow* const window) {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        const auto create_result = glfwCreateWindowSurface(
            instance,
            window,
            nullptr,
            &surface
        );

        if (VK_SUCCESS != create_result)
            return nullptr;

        if (VK_NULL_HANDLE == surface)
            return nullptr;

        return surface;
    }

}


namespace {

    std::function<void(int, int)> g_callback_func_buf_resize;

    static void callback_fbuf_resize(GLFWwindow* window, int width, int height) {
        if (g_callback_func_buf_resize)
            g_callback_func_buf_resize(width, height);
    }


    std::function<void(const dal::MouseEvent&)> g_callback_func_mouse_event;

    void callback_cursor_pos(GLFWwindow* window, double xpos, double ypos) {
        if (!g_callback_func_mouse_event)
            return;

        dal::MouseEvent e;
        e.m_action_type = dal::MouseActionType::move;
        e.m_time_sec = dal::get_cur_sec();
        e.m_pos = glm::vec2{ xpos, ypos };

        g_callback_func_mouse_event(e);
    }

    void callback_mouse_button(GLFWwindow* window, int button, int action, int mods) {
        if (!g_callback_func_mouse_event)
            return;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        dal::MouseEvent e;
        e.m_time_sec = dal::get_cur_sec();
        e.m_pos = glm::vec2{ xpos, ypos };

        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                e.m_button = dal::MouseButton::left;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                e.m_button = dal::MouseButton::right;
                break;
        }

        switch (action) {
            case GLFW_PRESS:
                e.m_action_type = dal::MouseActionType::down;
                break;
            case GLFW_RELEASE:
                e.m_action_type = dal::MouseActionType::up;
                break;
        }

        g_callback_func_mouse_event(e);
    }

}


namespace dal {

    WindowGLFW::WindowGLFW(const char* const title) {
        this->m_title = title;

        const auto window = ::create_glfw_window(800, 450, title);
        if (nullptr == window) {
            throw std::runtime_error{ "Failed to create glfw window" };
        }
        this->m_window = window;

        glfwSetFramebufferSizeCallback(window, ::callback_fbuf_resize);
        glfwSetCursorPosCallback(window, ::callback_cursor_pos);
        glfwSetMouseButtonCallback(window, ::callback_mouse_button);
    }

    WindowGLFW::~WindowGLFW() {
        glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(this->m_window));
        this->m_window = nullptr;
        glfwTerminate();
    }

    void WindowGLFW::do_frame() {
        glfwPollEvents();
    }

    bool WindowGLFW::should_close() const {
        return glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(this->m_window));
    }

    std::vector<const char*> WindowGLFW::get_vulkan_extensions() const {
        uint32_t ext_count = 0;
        const char** glfw_extensions = nullptr;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&ext_count);
        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + ext_count);

        return extensions;
    }

    std::function<void*(void*)> WindowGLFW::get_vk_surface_creator() const {
        return [this](void* vk_instance) -> void* {
            return ::create_vk_surface(
                reinterpret_cast<VkInstance>(vk_instance),
                reinterpret_cast<GLFWwindow*>(this->m_window)
            );
        };
    }

    void WindowGLFW::set_callback_fbuf_resize(std::function<void(int, int)> func) {
        g_callback_func_buf_resize = func;
    }

    void WindowGLFW::set_callback_mouse_event(std::function<void(const dal::MouseEvent&)> func) {
        g_callback_func_mouse_event = func;
    }

    uint32_t WindowGLFW::width() const {
        int width = 0;
        glfwGetFramebufferSize(reinterpret_cast<GLFWwindow*>(this->m_window), &width, nullptr);
        return width;
    }

    uint32_t WindowGLFW::height() const {
        int height = 0;
        glfwGetFramebufferSize(reinterpret_cast<GLFWwindow*>(this->m_window), nullptr, &height);
        return height;
    }

}
