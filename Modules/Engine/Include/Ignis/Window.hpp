#pragma once

#include <Ignis/Window/Enums.hpp>
#include <Ignis/Window/Events.hpp>

namespace Ignis {
    class Window {
       public:
        struct Settings {
            std::string_view Title = "Ignis::Window";

            uint32_t Width  = 1280;
            uint32_t Height = 800;

            bool Resizable  = true;
            bool Fullscreen = false;

            bool FullscreenMonitorSize = true;
        };

       public:
        static void Initialize(Window *window, const Settings &settings);
        static void Shutdown();

        static void PollEvents(WindowEvents *events);

        static void SetTitle(std::string_view title);
        static void SetSize(uint32_t width, uint32_t height);
        static void SetWidth(uint32_t width);
        static void SetHeight(uint32_t height);
        static void SetPosition(uint32_t x, uint32_t y);
        static void SetX(uint32_t x);
        static void SetY(uint32_t y);
        static void SetMousePosition(double x, double y);
        static void SetMouseX(double x);
        static void SetMouseY(double y);
        static void SetCursorMode(CursorMode mode);
        static void SetResizable(bool resizable);
        static void SetRunning(bool running);
        static void SetStickyKeys(bool sticky_keys = true);
        static void SetLockKeyMods(bool lock_key_mods = true);
        static void SetStickyMouseButtons(bool sticky_mouse_buttons = true);
        static void SetRawMouseMotion(bool raw_mouse_motion = true);
        static void MakeFullscreen(bool monitor_size = true);
        static void MakeWindowed();
        static void Maximize();
        static void Minimize();
        static void Restore();
        static void Close();

        static void RequestAttention();

        static GLFWwindow                   *Get();
        static std::string_view              GetTitle();
        static std::pair<uint32_t, uint32_t> GetSize();
        static uint32_t                      GetWidth();
        static uint32_t                      GetHeight();
        static std::pair<uint32_t, uint32_t> GetPosition();
        static uint32_t                      GetX();
        static uint32_t                      GetY();
        static std::pair<double, double>     GetMousePosition();
        static double                        GetMouseX();
        static double                        GetMouseY();
        static CursorMode                    GetCursorMode();

        static bool IsClosed();
        static bool IsRunning();
        static bool IsMaximized();
        static bool IsMinimized();
        static bool IsFocused();
        static bool IsFullscreen();
        static bool IsResizable();

        static bool IsStickyKeys();
        static bool IsLockKeyMods();
        static bool IsStickyMouseButtons();
        static bool IsRawMouseMotion();

        static bool IsKeyModPressed(KeyMod::Flags mod);
        static bool IsKeyDown(KeyCode key);
        static bool IsKeyUp(KeyCode key);
        static bool IsMouseButtonDown(MouseButton button);
        static bool IsMouseButtonUp(MouseButton button);

        static bool IsRawMouseMotionSupported();

       public:
        Window()  = default;
        ~Window() = default;

       private:
        IGNIS_IF_DEBUG(struct State {
           public:
            ~State();
        };);

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

       private:
        static Window *s_pInstance;

        static WindowEvents *s_pEvents;

        IGNIS_IF_DEBUG(static State s_State;)
    };
}  // namespace Ignis