#include <Ignis/Window.hpp>

namespace Ignis {
    void Window::GlfwErrorCallback(
        const int32_t error_code,
        const char   *description) {
        static std::string error_name{};
        error_name.clear();

        switch (error_code) {
            case GLFW_NO_ERROR:
                error_name = "GLFW_NO_ERROR";
                break;
            case GLFW_NOT_INITIALIZED:
                error_name = "GLFW_NOT_INITIALIZED";
                break;
            case GLFW_NO_CURRENT_CONTEXT:
                error_name = "GLFW_NO_CURRENT_CONTEXT";
                break;
            case GLFW_INVALID_ENUM:
                error_name = "GLFW_INVALID_ENUM";
                break;
            case GLFW_INVALID_VALUE:
                error_name = "GLFW_INVALID_VALUE";
                break;
            case GLFW_OUT_OF_MEMORY:
                error_name = "GLFW_OUT_OF_MEMORY";
                break;
            case GLFW_API_UNAVAILABLE:
                error_name = "GLFW_API_UNAVAILABLE";
                break;
            case GLFW_VERSION_UNAVAILABLE:
                error_name = "GLFW_VERSION_UNAVAILABLE";
                break;
            case GLFW_PLATFORM_ERROR:
                error_name = "GLFW_PLATFORM_ERROR";
                break;
            case GLFW_FORMAT_UNAVAILABLE:
                error_name = "GLFW_FORMAT_UNAVAILABLE";
                break;
            case GLFW_NO_WINDOW_CONTEXT:
                error_name = "GLFW_NO_WINDOW_CONTEXT";
                break;
            default:
                error_name = "GLFW_UNKNOWN_ERROR";
                break;
        }

        DIGNIS_LOG_ENGINE_ERROR("Glfw Error ({}): '{}'", error_name, description);
    }

    void Window::GlfwWindowCloseCallback(GLFWwindow *window) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        ignis_window->postEvent<WindowCloseEvent>();
    }

    void Window::GlfwWindowSizeCallback(GLFWwindow *window, const int32_t width, const int32_t height) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        ignis_window->postEvent<WindowResizeEvent>(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    }

    void Window::GlfwWindowContentScaleCallback(GLFWwindow *window, const float x_scale, const float y_scale) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        ignis_window->postEvent<WindowContentScaleEvent>(x_scale, y_scale);
    }

    void Window::GlfwWindowPositionCallback(GLFWwindow *window, const int32_t x, const int32_t y) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        ignis_window->postEvent<WindowMoveEvent>(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
    }

    void Window::GlfwWindowMaximizeCallback(GLFWwindow *window, const int32_t is_maximized) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));

        if (is_maximized == GLFW_TRUE) {
            ignis_window->postEvent<WindowMaximizeEvent>();
        } else {
            ignis_window->postEvent<WindowRestoreEvent>();
        }
    }

    void Window::GlfwWindowMinimizeCallback(GLFWwindow *window, const int32_t is_minimized) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));

        if (is_minimized == GLFW_TRUE) {
            ignis_window->postEvent<WindowMinimizeEvent>();
        } else {
            ignis_window->postEvent<WindowRestoreEvent>();
        }
    }

    void Window::GlfwWindowFocusCallback(GLFWwindow *window, const int32_t focused) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));

        if (focused == GLFW_TRUE) {
            ignis_window->postEvent<WindowGainFocusEvent>();
        } else {
            ignis_window->postEvent<WindowLoseFocusEvent>();
        }
    }

    void Window::GlfwWindowRefreshCallback(GLFWwindow *window) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        ignis_window->postEvent<WindowRefreshEvent>();
    }

    void Window::GlfwWindowKeyCallback(
        GLFWwindow   *window,
        const int32_t key,
        const int32_t /*scancode*/,
        const int32_t action,
        const int32_t mods) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));

        ignis_window->m_KeyMods   = mods;
        ignis_window->m_Keys[key] = action != GLFW_RELEASE;

        ignis_window->postEvent<WindowKeyEvent>(
            static_cast<KeyCode>(key),
            static_cast<KeyMod::Flags>(mods),
            static_cast<KeyAction>(action));
    }

    void Window::GlfwWindowCharCallback(GLFWwindow *window, const uint32_t codepoint) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        ignis_window->postEvent<WindowCharEvent>(codepoint);
    }

    void Window::GlfwWindowMousePositionCallback(GLFWwindow *window, const double x, const double y) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        ignis_window->postEvent<WindowMouseMoveEvent>(x, y);
    }

    void Window::GlfwWindowMouseEnterCallback(GLFWwindow *window, const int32_t entered) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        if (entered == GLFW_TRUE) {
            ignis_window->postEvent<WindowMouseEnterEvent>();
        } else {
            ignis_window->postEvent<WindowMouseLeaveEvent>();
        }
    }

    void Window::GlfwWindowMouseButtonCallback(
        GLFWwindow   *window,
        const int32_t button,
        const int32_t action,
        const int32_t mods) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));

        ignis_window->m_KeyMods              = mods;
        ignis_window->m_MouseButtons[button] = action != GLFW_RELEASE;

        ignis_window->postEvent<WindowMouseButtonEvent>(
            static_cast<MouseButton>(button),
            static_cast<KeyMod::Flags>(mods),
            static_cast<KeyAction>(action));
    }

    void Window::GlfwWindowMouseScrollCallback(GLFWwindow *window, const double x_offset, const double y_offset) {
        const auto ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        ignis_window->postEvent<WindowMouseScrollEvent>(x_offset, y_offset);
    }

    void Window::GlfwWindowDropCallback(GLFWwindow *window, const int32_t count, const char **paths) {
        Window                  *ignis_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
        std::vector<std::string> event_paths{};
        event_paths.reserve(count);

        for (uint32_t i = 0; i < count; i++) {
            event_paths.emplace_back(paths[i]);
        }

        ignis_window->postEvent<WindowDropEvent>(std::move(event_paths));
    }
}  // namespace Ignis