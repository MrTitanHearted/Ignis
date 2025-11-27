#pragma once

#include <Ignis/Engine/Layer.hpp>

#include <Ignis/Engine/WindowLayer/Enums.hpp>
#include <Ignis/Engine/WindowLayer/Events.hpp>

namespace Ignis {
    class WindowLayer final : public ILayer {
       public:
        struct Settings {
            std::string_view Title = "Ignis::Window";

            uint32_t Width  = 1280;
            uint32_t Height = 800;

            bool Resizable  = true;
            bool Fullscreen = false;

            bool FullscreenMonitorSize = true;

            std::optional<std::string_view> Icon = std::nullopt;
        };

       public:
        static void PollEvents();
        static void WaitEvents();

        static bool IsRawMouseMotionSupported();

       public:
        explicit WindowLayer(const Settings &settings);
        ~WindowLayer() override;

        void setIcon(std::string_view path) const;

        void setTitle(std::string_view title) const;
        void setSize(uint32_t width, uint32_t height) const;
        void setPosition(uint32_t x, uint32_t y) const;
        void setMousePosition(double x, double y) const;
        void setWidth(uint32_t width) const;
        void setHeight(uint32_t height) const;
        void setX(uint32_t x) const;
        void setY(uint32_t y) const;
        void setMouseX(double x) const;
        void setMouseY(double y) const;
        void setCursorMode(CursorMode mode) const;
        void setResizable(bool resizable) const;
        void setRunning(bool running) const;
        void setStickyKeys(bool sticky_keys = true) const;
        void setLockKeyMods(bool lock_key_mods = true) const;
        void setStickyMouseButtons(bool sticky_mouse_buttons = true) const;
        void setRawMouseMotion(bool raw_mouse_motion = true) const;
        void makeFullscreen(bool monitor_size = true);
        void makeWindowed() const;
        void maximize() const;
        void minimize() const;
        void restore() const;
        void close() const;

        void requestAttention() const;

        GLFWwindow      *get() const;
        std::string_view getTitle() const;

        std::pair<uint32_t, uint32_t> getSize() const;
        std::pair<uint32_t, uint32_t> getPosition() const;
        std::pair<double, double>     getMousePosition() const;

        uint32_t   getWidth() const;
        uint32_t   getHeight() const;
        uint32_t   getX() const;
        uint32_t   getY() const;
        double     getMouseX() const;
        double     getMouseY() const;
        CursorMode getCursorMode() const;

        bool isClosed() const;
        bool isRunning() const;
        bool isMaximized() const;
        bool isMinimized() const;
        bool isFocused() const;
        bool isFullscreen() const;
        bool isResizable() const;

        bool isStickyKeys() const;
        bool isLockKeyMods() const;
        bool isStickyMouseButtons() const;
        bool isRawMouseMotion() const;

        bool isKeyModPressed(KeyMod::Flags mod) const;
        bool isKeyDown(KeyCode key) const;
        bool isKeyUp(KeyCode key) const;
        bool isMouseButtonDown(MouseButton button) const;
        bool isMouseButtonUp(MouseButton button) const;

       private:
        static void GlfwErrorCallback(int32_t error_code, const char *description);

        static void GlfwWindowCloseCallback(GLFWwindow *window);
        static void GlfwWindowSizeCallback(GLFWwindow *window, int32_t width, int32_t height);
        static void GlfwWindowContentScaleCallback(GLFWwindow *window, float x_scale, float y_scale);
        static void GlfwWindowPositionCallback(GLFWwindow *window, int32_t x, int32_t y);
        static void GlfwWindowMaximizeCallback(GLFWwindow *window, int32_t is_maximized);
        static void GlfwWindowMinimizeCallback(GLFWwindow *window, int32_t is_minimized);
        static void GlfwWindowFocusCallback(GLFWwindow *window, int32_t focused);
        static void GlfwWindowRefreshCallback(GLFWwindow *window);
        static void GlfwWindowKeyCallback(GLFWwindow *window, int32_t key, int32_t scancode, int32_t action, int32_t mods);
        static void GlfwWindowCharCallback(GLFWwindow *window, uint32_t codepoint);
        static void GlfwWindowMousePositionCallback(GLFWwindow *window, double x, double y);
        static void GlfwWindowMouseEnterCallback(GLFWwindow *window, int32_t entered);
        static void GlfwWindowMouseButtonCallback(GLFWwindow *window, int32_t button, int32_t action, int32_t mods);
        static void GlfwWindowMouseScrollCallback(GLFWwindow *window, double x_offset, double y_offset);
        static void GlfwWindowDropCallback(GLFWwindow *window, int32_t count, const char **paths);

       private:
        GLFWwindow *m_pWindow = nullptr;

        std::uint32_t m_PrevWidth;
        std::uint32_t m_PrevHeight;
        std::uint32_t m_PrevX;
        std::uint32_t m_PrevY;

        KeyMod::Flags m_KeyMods;

        std::array<bool, GLFW_KEY_LAST + 1>          m_Keys;
        std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_MouseButtons;
    };
}  // namespace Ignis