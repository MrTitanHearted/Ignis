#include <Ignis/Window.hpp>

namespace Ignis {
    void Window::GlfwErrorCallback(int32_t error_code, const char *description) {
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

    void Window::GlfwWindowCloseCallback(GLFWwindow *) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        s_pEvents->addCloseEvent(WindowCloseEvent{});
    }

    void Window::GlfwWindowSizeCallback(GLFWwindow *, const int32_t width, const int32_t height) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        s_pEvents->addResizeEvent(WindowResizeEvent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    }

    void Window::GlfwWindowContentScaleCallback(GLFWwindow *, float x_scale, float y_scale) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        s_pEvents->addContentScaleEvent(WindowContentScaleEvent{x_scale, y_scale});
    }

    void Window::GlfwWindowPositionCallback(GLFWwindow *, const int32_t x, const int32_t y) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        s_pEvents->addMoveEvent(WindowMoveEvent{static_cast<uint32_t>(x), static_cast<uint32_t>(y)});
    }

    void Window::GlfwWindowMaximizeCallback(GLFWwindow *, const int32_t is_maximized) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        if (is_maximized == GLFW_TRUE)
            s_pEvents->addMaximizeEvent(WindowMaximizeEvent{});
        else
            s_pEvents->addRestoreEvent(WindowRestoreEvent{});
    }

    void Window::GlfwWindowMinimizeCallback(GLFWwindow *, const int32_t is_minimized) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");

        if (is_minimized == GLFW_TRUE)
            s_pEvents->addMinimizeEvent(WindowMinimizeEvent{});
        else
            s_pEvents->addRestoreEvent(WindowRestoreEvent{});
    }

    void Window::GlfwWindowFocusCallback(GLFWwindow *, const int32_t focused) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");

        if (focused == GLFW_TRUE)
            s_pEvents->addGainFocusEvent(WindowGainFocusEvent{});
        else
            s_pEvents->addLoseFocusEvent(WindowLoseFocusEvent{});
    }

    void Window::GlfwWindowRefreshCallback(GLFWwindow *) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        s_pEvents->addRefreshEvent(WindowRefreshEvent{});
    }

    void Window::GlfwWindowKeyCallback(
        GLFWwindow *,
        const int32_t key,
        const int32_t /*scancode*/,
        const int32_t action,
        const int32_t mods) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Window is not initialized.");

        s_pInstance->m_KeyMods   = mods;
        s_pInstance->m_Keys[key] = action == GLFW_PRESS;

        s_pEvents->addKeyEvent(WindowKeyEvent{
            static_cast<KeyCode>(key),
            static_cast<KeyMod::Flags>(mods),
            static_cast<KeyAction>(action),
        });
    }

    void Window::GlfwWindowCharCallback(GLFWwindow *, const uint32_t codepoint) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        s_pEvents->addCharEvent(WindowCharEvent{codepoint});
    }

    void Window::GlfwWindowMousePositionCallback(GLFWwindow *, const double x, const double y) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        s_pEvents->addMouseMoveEvent(WindowMouseMoveEvent{x, y});
    }

    void Window::GlfwWindowMouseEnterCallback(GLFWwindow *, const int32_t entered) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");

        if (entered == GLFW_TRUE)
            s_pEvents->addMouseEnterEvent(WindowMouseEnterEvent{});
        else
            s_pEvents->addMouseLeaveEvent(WindowMouseLeaveEvent{});
    }

    void Window::GlfwWindowMouseButtonCallback(
        GLFWwindow *,
        const int32_t button,
        const int32_t action,
        const int32_t mods) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Window is not initialized.");

        s_pInstance->m_KeyMods              = mods;
        s_pInstance->m_MouseButtons[button] = action == GLFW_PRESS;

        s_pEvents->addMouseButtonEvent(WindowMouseButtonEvent{
            static_cast<MouseButton>(button),
            static_cast<KeyMod::Flags>(mods),
            static_cast<KeyAction>(action),
        });
    }

    void Window::GlfwWindowMouseScrollCallback(GLFWwindow *, const double x_offset, const double y_offset) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");
        s_pEvents->addMouseScrollEvent(WindowMouseScrollEvent{x_offset, y_offset});
    }

    void Window::GlfwWindowDropCallback(GLFWwindow *, const int32_t count, const char **paths) {
        DIGNIS_ASSERT(s_pEvents != nullptr, "How is Ignis::Window::s_pEvents not initialized?");

        std::vector<std::string> event{static_cast<size_t>(count)};

        for (uint32_t i = 0; i < count; i++) {
            event.emplace_back(paths[i]);
        }

        s_pEvents->addDropEvent(WindowDropEvent{std::move(event)});
    }

    void WindowEvents::clear() {
        m_Events.clear();
    }

    std::span<const WindowEvent> WindowEvents::getEvents() const {
        return m_Events;
    }

    std::vector<WindowEvent>::const_iterator WindowEvents::begin() const {
        return m_Events.begin();
    }

    std::vector<WindowEvent>::const_iterator WindowEvents::end() const {
        return m_Events.end();
    }

    void WindowEvents::addCloseEvent(WindowCloseEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addResizeEvent(WindowResizeEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addContentScaleEvent(WindowContentScaleEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addMoveEvent(WindowMoveEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addRestoreEvent(WindowRestoreEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addMinimizeEvent(WindowMinimizeEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addMaximizeEvent(WindowMaximizeEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addGainFocusEvent(WindowGainFocusEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addLoseFocusEvent(WindowLoseFocusEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addRefreshEvent(WindowRefreshEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addKeyEvent(WindowKeyEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addCharEvent(WindowCharEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addMouseMoveEvent(WindowMouseMoveEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addMouseEnterEvent(WindowMouseEnterEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addMouseLeaveEvent(WindowMouseLeaveEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addMouseButtonEvent(WindowMouseButtonEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addMouseScrollEvent(WindowMouseScrollEvent &&event) {
        m_Events.emplace_back(event);
    }

    void WindowEvents::addDropEvent(WindowDropEvent &&event) {
        m_Events.emplace_back(event);
    }
}  // namespace Ignis