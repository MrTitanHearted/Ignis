#include <Ignis/Window.hpp>

namespace Ignis {
    Window *Window::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Window::State Window::s_State{};)

    Window &Window::GetRef() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return *s_pInstance;
    }

    void Window::PollEvents() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwPollEvents();
    }

    void Window::WaitEvents() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwWaitEvents();
    }

    bool Window::IsRawMouseMotionSupported() {
        return glfwRawMouseMotionSupported() == GLFW_TRUE;
    }

    void Window::SetIcon(const std::string_view path) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        const auto texture_asset_opt = TextureAsset::LoadFromPath(path, TextureAsset::Type::eRGBA8u);
        DIGNIS_ASSERT(texture_asset_opt.has_value(), "Failed to load icon from path: {}.", path);

        const TextureAsset &texture_asset = texture_asset_opt.value();

        GLFWimage icon{};
        icon.width  = static_cast<int32_t>(texture_asset.getWidth());
        icon.height = static_cast<int32_t>(texture_asset.getHeight());
        icon.pixels = const_cast<unsigned char *>(texture_asset.getData().data());

        // ReSharper disable once CppDFANullDereference
        glfwSetWindowIcon(s_pInstance->m_pWindow, 1, &icon);
    }

    void Window::SetTitle(const std::string_view title) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetWindowTitle(s_pInstance->m_pWindow, title.data());
    }

    void Window::SetSize(const uint32_t width, const uint32_t height) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetWindowSize(s_pInstance->m_pWindow, static_cast<int32_t>(width), static_cast<int32_t>(height));
    }

    void Window::SetPosition(const uint32_t x, const uint32_t y) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetWindowPos(s_pInstance->m_pWindow, static_cast<int32_t>(x), static_cast<int32_t>(y));
    }

    void Window::SetMousePosition(const double x, const double y) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetCursorPos(s_pInstance->m_pWindow, x, y);
    }

    void Window::SetWidth(const uint32_t width) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t height = 0;
        glfwGetWindowSize(s_pInstance->m_pWindow, nullptr, &height);
        glfwSetWindowSize(s_pInstance->m_pWindow, static_cast<int32_t>(width), height);
    }

    void Window::SetHeight(const uint32_t height) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t width = 0;
        glfwGetWindowSize(s_pInstance->m_pWindow, &width, nullptr);
        glfwSetWindowSize(s_pInstance->m_pWindow, width, static_cast<int32_t>(height));
    }

    void Window::SetX(const uint32_t x) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t y = 0;
        glfwGetWindowPos(s_pInstance->m_pWindow, nullptr, &y);
        glfwSetWindowPos(s_pInstance->m_pWindow, static_cast<int32_t>(x), y);
    }

    void Window::SetY(const uint32_t y) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t x = 0;
        glfwGetWindowPos(s_pInstance->m_pWindow, &x, nullptr);
        glfwSetWindowPos(s_pInstance->m_pWindow, x, static_cast<int32_t>(y));
    }

    void Window::SetMouseX(const double x) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        double y = 0.0;
        glfwGetCursorPos(s_pInstance->m_pWindow, nullptr, &y);
        glfwSetCursorPos(s_pInstance->m_pWindow, x, y);
    }

    void Window::SetMouseY(const double y) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        double x = 0.0;
        glfwGetCursorPos(s_pInstance->m_pWindow, &x, nullptr);
        glfwSetCursorPos(s_pInstance->m_pWindow, x, y);
    }

    void Window::SetCursorMode(CursorMode mode) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetInputMode(s_pInstance->m_pWindow, GLFW_CURSOR, static_cast<int32_t>(mode));
    }

    void Window::SetResizable(const bool resizable) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetWindowAttrib(s_pInstance->m_pWindow, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::SetRunning(const bool running) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetWindowShouldClose(s_pInstance->m_pWindow, running ? GLFW_FALSE : GLFW_TRUE);
    }

    void Window::SetStickyKeys(const bool sticky_keys) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetInputMode(s_pInstance->m_pWindow, GLFW_STICKY_KEYS, sticky_keys ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::SetLockKeyMods(const bool lock_key_mods) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetInputMode(s_pInstance->m_pWindow, GLFW_LOCK_KEY_MODS, lock_key_mods ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::SetStickyMouseButtons(const bool sticky_mouse_buttons) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetInputMode(s_pInstance->m_pWindow, GLFW_STICKY_MOUSE_BUTTONS, sticky_mouse_buttons ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::SetRawMouseMotion(const bool raw_mouse_motion) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        IGNIS_ASSERT(IsRawMouseMotionSupported(), "Current machine does not support raw mouse motion.");
        glfwSetInputMode(s_pInstance->m_pWindow, GLFW_RAW_MOUSE_MOTION, raw_mouse_motion ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::MakeFullscreen(const bool monitor_size) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t width  = 0;
        int32_t height = 0;
        int32_t x      = 0;
        int32_t y      = 0;

        glfwGetWindowSize(s_pInstance->m_pWindow, &width, &height);
        glfwGetWindowPos(s_pInstance->m_pWindow, &x, &y);

        s_pInstance->m_PrevWidth  = width;
        s_pInstance->m_PrevHeight = height;
        s_pInstance->m_PrevX      = x;
        s_pInstance->m_PrevY      = y;

        GLFWmonitor       *monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode    = glfwGetVideoMode(monitor);
        if (monitor_size) {
            width  = mode->width;
            height = mode->height;
        }

        glfwSetWindowMonitor(s_pInstance->m_pWindow, monitor, x, y, width, height, GLFW_DONT_CARE);
    }

    void Window::MakeWindowed() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetWindowMonitor(
            s_pInstance->m_pWindow,
            nullptr,
            static_cast<int32_t>(s_pInstance->m_PrevX),
            static_cast<int32_t>(s_pInstance->m_PrevY),
            static_cast<int32_t>(s_pInstance->m_PrevWidth),
            static_cast<int32_t>(s_pInstance->m_PrevHeight),
            GLFW_DONT_CARE);
    }

    void Window::Maximize() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwMaximizeWindow(s_pInstance->m_pWindow);
    }

    void Window::Minimize() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwIconifyWindow(s_pInstance->m_pWindow);
    }

    void Window::Restore() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwRestoreWindow(s_pInstance->m_pWindow);
    }

    void Window::Close() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwSetWindowShouldClose(s_pInstance->m_pWindow, GLFW_TRUE);
    }

    void Window::RequestAttention() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        glfwRequestWindowAttention(s_pInstance->m_pWindow);
    }

    GLFWwindow *Window::GetHandle() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return s_pInstance->m_pWindow;
    }

    std::string_view Window::GetTitle() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetWindowTitle(s_pInstance->m_pWindow);
    }

    std::pair<uint32_t, uint32_t> Window::GetSize() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t width  = 0;
        int32_t height = 0;
        glfwGetWindowSize(s_pInstance->m_pWindow, &width, &height);
        return std::make_pair(width, height);
    }

    std::pair<uint32_t, uint32_t> Window::GetPosition() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t x = 0;
        int32_t y = 0;
        glfwGetWindowPos(s_pInstance->m_pWindow, &x, &y);
        return std::make_pair(x, y);
    }

    std::pair<double, double> Window::GetMousePosition() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        double x = 0;
        double y = 0;
        glfwGetCursorPos(s_pInstance->m_pWindow, &x, &y);
        return std::make_pair(x, y);
    }

    uint32_t Window::GetWidth() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t width = 0;
        glfwGetWindowSize(s_pInstance->m_pWindow, &width, nullptr);
        return width;
    }

    uint32_t Window::GetHeight() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t height = 0;
        glfwGetWindowSize(s_pInstance->m_pWindow, nullptr, &height);
        return height;
    }

    uint32_t Window::GetX() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t x = 0;
        glfwGetWindowPos(s_pInstance->m_pWindow, &x, nullptr);
        return x;
    }

    uint32_t Window::GetY() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        int32_t y = 0;
        glfwGetWindowPos(s_pInstance->m_pWindow, nullptr, &y);
        return y;
    }

    double Window::GetMouseX() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        double x = 0;
        glfwGetCursorPos(s_pInstance->m_pWindow, &x, nullptr);
        return x;
    }

    double Window::GetMouseY() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        double y = 0;
        glfwGetCursorPos(s_pInstance->m_pWindow, nullptr, &y);
        return y;
    }

    CursorMode Window::GetCursorMode() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return static_cast<CursorMode>(glfwGetInputMode(s_pInstance->m_pWindow, GLFW_CURSOR));
    }

    bool Window::IsClosed() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwWindowShouldClose(s_pInstance->m_pWindow) == GLFW_TRUE;
    }

    bool Window::IsRunning() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwWindowShouldClose(s_pInstance->m_pWindow) == GLFW_FALSE;
    }

    bool Window::IsMaximized() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetWindowAttrib(s_pInstance->m_pWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
    }

    bool Window::IsMinimized() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetWindowAttrib(s_pInstance->m_pWindow, GLFW_ICONIFIED) == GLFW_TRUE;
    }

    bool Window::IsFocused() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetWindowAttrib(s_pInstance->m_pWindow, GLFW_FOCUSED) == GLFW_TRUE;
    }

    bool Window::IsFullscreen() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetWindowMonitor(s_pInstance->m_pWindow) != nullptr;
    }

    bool Window::IsResizable() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetWindowAttrib(s_pInstance->m_pWindow, GLFW_RESIZABLE) == GLFW_TRUE;
    }

    bool Window::IsStickyKeys() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetInputMode(s_pInstance->m_pWindow, GLFW_STICKY_KEYS) == GLFW_TRUE;
    }

    bool Window::IsLockKeyMods() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetInputMode(s_pInstance->m_pWindow, GLFW_LOCK_KEY_MODS) == GLFW_TRUE;
    }

    bool Window::IsStickyMouseButtons() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetInputMode(s_pInstance->m_pWindow, GLFW_STICKY_MOUSE_BUTTONS) == GLFW_TRUE;
    }

    bool Window::IsRawMouseMotion() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return glfwGetInputMode(s_pInstance->m_pWindow, GLFW_RAW_MOUSE_MOTION) == GLFW_TRUE;
    }

    bool Window::IsKeyModPressed(const KeyMod::Flags mod) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return (s_pInstance->m_KeyMods & mod) > 0;
    }

    bool Window::IsKeyDown(const KeyCode key) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return s_pInstance->m_Keys[static_cast<uint32_t>(key)];
    }

    bool Window::IsKeyUp(const KeyCode key) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return !s_pInstance->m_Keys[static_cast<uint32_t>(key)];
    }

    bool Window::IsMouseButtonDown(const MouseButton button) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return s_pInstance->m_MouseButtons[static_cast<uint32_t>(button)];
    }

    bool Window::IsMouseButtonUp(const MouseButton button) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");
        return !s_pInstance->m_MouseButtons[static_cast<uint32_t>(button)];
    }

    void Window::initialize(const Settings &settings) {
        DIGNIS_ASSERT(nullptr == s_pInstance, "Ignis::Window is already initialized");
        IGNIS_ASSERT(glfwInit() != GLFW_FALSE, "Failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, settings.Resizable ? GLFW_TRUE : GLFW_FALSE);

        uint32_t width  = settings.Width;
        uint32_t height = settings.Height;

        GLFWmonitor *monitor = settings.Fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        if (settings.Fullscreen && settings.FullscreenMonitorSize) {
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);

            width  = mode->width;
            height = mode->height;
        }

        m_pWindow = glfwCreateWindow(
            static_cast<int32_t>(width),
            static_cast<int32_t>(height),
            settings.Title.data(),
            monitor, nullptr);

        glfwSetWindowUserPointer(m_pWindow, this);

        glfwSetWindowCloseCallback(m_pWindow, GlfwWindowCloseCallback);
        glfwSetWindowSizeCallback(m_pWindow, GlfwWindowSizeCallback);
        glfwSetWindowPosCallback(m_pWindow, GlfwWindowPositionCallback);
        glfwSetWindowContentScaleCallback(m_pWindow, GlfwWindowContentScaleCallback);
        glfwSetWindowPosCallback(m_pWindow, GlfwWindowPositionCallback);
        glfwSetWindowMaximizeCallback(m_pWindow, GlfwWindowMaximizeCallback);
        glfwSetWindowIconifyCallback(m_pWindow, GlfwWindowMinimizeCallback);
        glfwSetWindowFocusCallback(m_pWindow, GlfwWindowFocusCallback);
        glfwSetWindowRefreshCallback(m_pWindow, GlfwWindowRefreshCallback);
        glfwSetKeyCallback(m_pWindow, GlfwWindowKeyCallback);
        glfwSetCharCallback(m_pWindow, GlfwWindowCharCallback);
        glfwSetCursorPosCallback(m_pWindow, GlfwWindowMousePositionCallback);
        glfwSetCursorEnterCallback(m_pWindow, GlfwWindowMouseEnterCallback);
        glfwSetMouseButtonCallback(m_pWindow, GlfwWindowMouseButtonCallback);
        glfwSetScrollCallback(m_pWindow, GlfwWindowMouseScrollCallback);
        glfwSetDropCallback(m_pWindow, GlfwWindowDropCallback);

        glfwShowWindow(m_pWindow);

        m_KeyMods = KeyMod::eNone;
        std::fill(std::execution::par, std::begin(m_Keys),
                  std::end(m_Keys), false);
        std::fill(std::execution::par, std::begin(m_MouseButtons),
                  std::end(m_MouseButtons), false);

        if (monitor != nullptr) {
            m_PrevWidth  = width;
            m_PrevHeight = height;

            const GLFWvidmode *mode = glfwGetVideoMode(monitor);

            const uint32_t monitor_width  = mode->width;
            const uint32_t monitor_height = mode->height;

            m_PrevX = (monitor_width - width) / 2;
            m_PrevY = (monitor_height - height) / 2;
        }

        s_pInstance = this;

        DIGNIS_LOG_ENGINE_INFO("Ignis::Window Initialized");

        SetStickyKeys(false);
        SetStickyMouseButtons(false);
        SetLockKeyMods(true);
        SetRawMouseMotion(true);

        if (!settings.Icon.empty())
            SetIcon(settings.Icon);
    }

    void Window::shutdown() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Window is not initialized.");

        glfwDestroyWindow(m_pWindow);
        glfwTerminate();

        m_pWindow = nullptr;

        DIGNIS_LOG_ENGINE_INFO("Ignis::Window Shutdown");

        s_pInstance = nullptr;
    }

    IGNIS_IF_DEBUG(Window::State::~State() {
        assert(nullptr == s_pInstance && "Forgot to shutdown Ignis::Window.");
    });
}  // namespace Ignis