#include <Ignis/Engine/WindowLayer.hpp>

#include <Ignis/Engine.hpp>

namespace Ignis {
    void WindowLayer::GlfwErrorCallback(
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

    void WindowLayer::GlfwWindowCloseCallback(GLFWwindow *) {
        WindowCloseEvent event{};
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowSizeCallback(GLFWwindow *, const int32_t width, const int32_t height) {
        WindowResizeEvent event{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowContentScaleCallback(GLFWwindow *, const float x_scale, const float y_scale) {
        WindowContentScaleEvent event{x_scale, y_scale};
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowPositionCallback(GLFWwindow *, const int32_t x, const int32_t y) {
        WindowMoveEvent event{static_cast<uint32_t>(x), static_cast<uint32_t>(y)};
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowMaximizeCallback(GLFWwindow *, const int32_t is_maximized) {
        if (is_maximized == GLFW_TRUE) {
            WindowMaximizeEvent event{};
            Engine::Get().raiseEvent(event);
        } else {
            WindowRestoreEvent event{};
            Engine::Get().raiseEvent(event);
        }
    }

    void WindowLayer::GlfwWindowMinimizeCallback(GLFWwindow *, const int32_t is_minimized) {
        if (is_minimized == GLFW_TRUE) {
            WindowMinimizeEvent event{};
            Engine::Get().raiseEvent(event);
        } else {
            WindowRestoreEvent event{};
            Engine::Get().raiseEvent(event);
        }
    }

    void WindowLayer::GlfwWindowFocusCallback(GLFWwindow *, const int32_t focused) {
        if (focused == GLFW_TRUE) {
            WindowGainFocusEvent event{};
            Engine::Get().raiseEvent(event);
        } else {
            WindowLoseFocusEvent event{};
            Engine::Get().raiseEvent(event);
        }
    }

    void WindowLayer::GlfwWindowRefreshCallback(GLFWwindow *) {
        WindowRefreshEvent event{};
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowKeyCallback(
        GLFWwindow   *window,
        const int32_t key,
        const int32_t /*scancode*/,
        const int32_t action,
        const int32_t mods) {
        const auto layer = static_cast<WindowLayer *>(glfwGetWindowUserPointer(window));
        DIGNIS_ASSERT(layer != nullptr, "Ignis::WindowLayer is not initialized.");

        layer->m_KeyMods   = mods;
        layer->m_Keys[key] = action == GLFW_PRESS;

        WindowKeyEvent event{
            static_cast<KeyCode>(key),
            static_cast<KeyMod::Flags>(mods),
            static_cast<KeyAction>(action),
        };
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowCharCallback(GLFWwindow *, const uint32_t codepoint) {
        WindowCharEvent event{codepoint};
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowMousePositionCallback(GLFWwindow *, const double x, const double y) {
        WindowMouseMoveEvent event{x, y};
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowMouseEnterCallback(GLFWwindow *, const int32_t entered) {
        if (entered == GLFW_TRUE) {
            WindowMouseEnterEvent evnet{};
            Engine::Get().raiseEvent(evnet);
        } else {
            WindowMouseLeaveEvent event{};
            Engine::Get().raiseEvent(event);
        }
    }

    void WindowLayer::GlfwWindowMouseButtonCallback(
        GLFWwindow   *window,
        const int32_t button,
        const int32_t action,
        const int32_t mods) {
        const auto layer = static_cast<WindowLayer *>(glfwGetWindowUserPointer(window));
        DIGNIS_ASSERT(layer != nullptr, "Ignis::WindowLayer is not initialized.");

        layer->m_KeyMods              = mods;
        layer->m_MouseButtons[button] = action == GLFW_PRESS;

        WindowMouseButtonEvent event{
            static_cast<MouseButton>(button),
            static_cast<KeyMod::Flags>(mods),
            static_cast<KeyAction>(action),
        };
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowMouseScrollCallback(GLFWwindow *, const double x_offset, const double y_offset) {
        WindowMouseScrollEvent event{x_offset, y_offset};
        Engine::Get().raiseEvent(event);
    }

    void WindowLayer::GlfwWindowDropCallback(GLFWwindow *, const int32_t count, const char **paths) {
        std::vector<std::string> event_paths{};
        event_paths.reserve(count);

        for (uint32_t i = 0; i < count; i++) {
            event_paths.emplace_back(paths[i]);
        }

        WindowDropEvent event{std::move(event_paths)};
        Engine::Get().raiseEvent(event);
    }
}  // namespace Ignis