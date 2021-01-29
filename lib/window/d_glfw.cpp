#include "d_glfw.h"

#include <stdexcept>

#include <GLFW/glfw3.h>


namespace {

    // The result might be nullptr
    GLFWwindow* create_glfw_window(const unsigned width, const unsigned height, const char* const title) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        return glfwCreateWindow(width, height, title, nullptr, nullptr);
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
        std::vector<const char*> result;

        uint32_t ext_count = 0;
        const char** glfw_extensions = nullptr;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&ext_count);
        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + ext_count);

        return result;
    }

}
