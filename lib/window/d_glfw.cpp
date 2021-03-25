#include "d_glfw.h"

#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


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


namespace dal {

    WindowGLFW::WindowGLFW(const char* const title) {
        this->m_title = title;

        this->m_window = ::create_glfw_window(800, 450, title);
        if (nullptr == this->m_window) {
            throw std::runtime_error{ "Failed to create glfw window" };
        }
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
