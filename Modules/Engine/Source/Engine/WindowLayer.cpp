#include <Ignis/Engine/WindowLayer.hpp>

#include <Ignis/Engine.hpp>

namespace Ignis {
    void WindowLayer::PollEvents() {
        glfwPollEvents();
    }

    void WindowLayer::WaitEvents() {
        glfwWaitEvents();
    }

    bool WindowLayer::IsRawMouseMotionSupported() {
        return glfwRawMouseMotionSupported() == GLFW_TRUE;
    }

    WindowLayer::WindowLayer(const Settings &settings) {
        IGNIS_ASSERT(glfwInit() == GLFW_TRUE, "Failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, settings.Resizable ? GLFW_TRUE : GLFW_FALSE);

        uint32_t     width   = settings.Width;
        uint32_t     height  = settings.Height;
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

        DIGNIS_LOG_ENGINE_INFO("Ignis::WindowLayer Initialized");

        setStickyKeys(false);
        setStickyMouseButtons(false);
        setLockKeyMods(true);
        setRawMouseMotion(true);

        if (settings.Icon.has_value()) {
            setIcon(settings.Icon.value());
        }
    }

    WindowLayer::~WindowLayer() {
        glfwDestroyWindow(m_pWindow);
        glfwTerminate();

        DIGNIS_LOG_ENGINE_INFO("Ignis::WindowLayer Released");

        m_pWindow    = nullptr;
        m_PrevWidth  = 0;
        m_PrevHeight = 0;
        m_PrevX      = 0;
        m_PrevY      = 0;
        m_KeyMods    = 0;
    }

    void WindowLayer::setIcon(const std::string_view path) const {
        const auto texture_asset_opt = TextureAsset::LoadFromPath(path, TextureAsset::Type::eRGBA8u);
        DIGNIS_ASSERT(texture_asset_opt.has_value(), "Failed to load icon");

        const TextureAsset texture_asset = texture_asset_opt.value();

        GLFWimage icon{};
        icon.width  = texture_asset.getWidth();
        icon.height = texture_asset.getHeight();
        icon.pixels = const_cast<unsigned char *>(texture_asset.getData().data());

        glfwSetWindowIcon(m_pWindow, 1, &icon);
    }

    void WindowLayer::setTitle(const std::string_view title) const {
        glfwSetWindowTitle(m_pWindow, title.data());
    }

    void WindowLayer::setSize(const uint32_t width, const uint32_t height) const {
        glfwSetWindowSize(m_pWindow, static_cast<int32_t>(width), static_cast<int32_t>(height));
    }

    void WindowLayer::setPosition(const uint32_t x, const uint32_t y) const {
        glfwSetWindowPos(m_pWindow, static_cast<int32_t>(x), static_cast<int32_t>(y));
    }

    void WindowLayer::setMousePosition(const double x, const double y) const {
        glfwSetCursorPos(m_pWindow, x, y);
    }

    void WindowLayer::setWidth(const uint32_t width) const {
        int32_t height = 0;
        glfwGetWindowSize(m_pWindow, nullptr, &height);
        glfwSetWindowSize(m_pWindow, static_cast<int32_t>(width), height);
    }

    void WindowLayer::setHeight(const uint32_t height) const {
        int32_t width = 0;
        glfwGetWindowSize(m_pWindow, &width, nullptr);
        glfwSetWindowSize(m_pWindow, width, static_cast<int32_t>(height));
    }

    void WindowLayer::setX(const uint32_t x) const {
        int32_t y = 0;
        glfwGetWindowPos(m_pWindow, nullptr, &y);
        glfwSetWindowPos(m_pWindow, static_cast<int32_t>(x), y);
    }

    void WindowLayer::setY(const uint32_t y) const {
        int32_t x = 0;
        glfwGetWindowPos(m_pWindow, &x, nullptr);
        glfwSetWindowPos(m_pWindow, x, static_cast<int32_t>(y));
    }

    void WindowLayer::setMouseX(const double x) const {
        double y = 0.0;
        glfwGetCursorPos(m_pWindow, nullptr, &y);
        glfwSetCursorPos(m_pWindow, x, y);
    }

    void WindowLayer::setMouseY(const double y) const {
        double x = 0.0;
        glfwGetCursorPos(m_pWindow, &x, nullptr);
        glfwSetCursorPos(m_pWindow, x, y);
    }

    void WindowLayer::setCursorMode(CursorMode mode) const {
        glfwSetInputMode(m_pWindow, GLFW_CURSOR, static_cast<int32_t>(mode));
    }

    void WindowLayer::setResizable(const bool resizable) const {
        glfwSetWindowAttrib(m_pWindow, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    }

    void WindowLayer::setRunning(const bool running) const {
        glfwSetWindowShouldClose(m_pWindow, running ? GLFW_FALSE : GLFW_TRUE);
    }

    void WindowLayer::setStickyKeys(const bool sticky_keys) const {
        glfwSetInputMode(m_pWindow, GLFW_STICKY_KEYS, sticky_keys ? GLFW_TRUE : GLFW_FALSE);
    }

    void WindowLayer::setLockKeyMods(const bool lock_key_mods) const {
        glfwSetInputMode(m_pWindow, GLFW_LOCK_KEY_MODS, lock_key_mods ? GLFW_TRUE : GLFW_FALSE);
    }

    void WindowLayer::setStickyMouseButtons(const bool sticky_mouse_buttons) const {
        glfwSetInputMode(m_pWindow, GLFW_STICKY_MOUSE_BUTTONS, sticky_mouse_buttons ? GLFW_TRUE : GLFW_FALSE);
    }

    void WindowLayer::setRawMouseMotion(const bool raw_mouse_motion) const {
        IGNIS_ASSERT(IsRawMouseMotionSupported(), "Current machine does not support raw mouse motion.");
        glfwSetInputMode(m_pWindow, GLFW_RAW_MOUSE_MOTION, raw_mouse_motion ? GLFW_TRUE : GLFW_FALSE);
    }

    void WindowLayer::makeFullscreen(const bool monitor_size) {
        int32_t width  = 0;
        int32_t height = 0;
        int32_t x      = 0;
        int32_t y      = 0;

        glfwGetWindowSize(m_pWindow, &width, &height);
        glfwGetWindowPos(m_pWindow, &x, &y);

        m_PrevWidth  = width;
        m_PrevHeight = height;
        m_PrevX      = x;
        m_PrevY      = y;

        GLFWmonitor       *monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode    = glfwGetVideoMode(monitor);
        if (monitor_size) {
            width  = mode->width;
            height = mode->height;
        }

        glfwSetWindowMonitor(m_pWindow, monitor, x, y, width, height, GLFW_DONT_CARE);
    }

    void WindowLayer::makeWindowed() const {
        glfwSetWindowMonitor(
            m_pWindow,
            nullptr,
            static_cast<int32_t>(m_PrevX),
            static_cast<int32_t>(m_PrevY),
            static_cast<int32_t>(m_PrevWidth),
            static_cast<int32_t>(m_PrevHeight),
            GLFW_DONT_CARE);
    }

    void WindowLayer::maximize() const {
        glfwMaximizeWindow(m_pWindow);
    }

    void WindowLayer::minimize() const {
        glfwIconifyWindow(m_pWindow);
    }

    void WindowLayer::restore() const {
        glfwRestoreWindow(m_pWindow);
    }

    void WindowLayer::close() const {
        glfwSetWindowShouldClose(m_pWindow, GLFW_TRUE);
    }

    void WindowLayer::requestAttention() const {
        glfwRequestWindowAttention(m_pWindow);
    }

    GLFWwindow *WindowLayer::get() const {
        return m_pWindow;
    }

    std::string_view WindowLayer::getTitle() const {
        return glfwGetWindowTitle(m_pWindow);
    }

    std::pair<uint32_t, uint32_t> WindowLayer::getSize() const {
        int32_t width  = 0;
        int32_t height = 0;
        glfwGetWindowSize(m_pWindow, &width, &height);
        return std::make_pair(width, height);
    }

    std::pair<uint32_t, uint32_t> WindowLayer::getPosition() const {
        int32_t x = 0;
        int32_t y = 0;
        glfwGetWindowPos(m_pWindow, &x, &y);
        return std::make_pair(x, y);
    }

    std::pair<double, double> WindowLayer::getMousePosition() const {
        double x = 0;
        double y = 0;
        glfwGetCursorPos(m_pWindow, &x, &y);
        return std::make_pair(x, y);
    }

    uint32_t WindowLayer::getWidth() const {
        int32_t width = 0;
        glfwGetWindowSize(m_pWindow, &width, nullptr);
        return width;
    }

    uint32_t WindowLayer::getHeight() const {
        int32_t height = 0;
        glfwGetWindowSize(m_pWindow, nullptr, &height);
        return height;
    }

    uint32_t WindowLayer::getX() const {
        int32_t x = 0;
        glfwGetWindowPos(m_pWindow, &x, nullptr);
        return x;
    }

    uint32_t WindowLayer::getY() const {
        int32_t y = 0;
        glfwGetWindowPos(m_pWindow, nullptr, &y);
        return y;
    }

    double WindowLayer::getMouseX() const {
        double x = 0;
        glfwGetCursorPos(m_pWindow, &x, nullptr);
        return x;
    }

    double WindowLayer::getMouseY() const {
        double y = 0;
        glfwGetCursorPos(m_pWindow, nullptr, &y);
        return y;
    }

    CursorMode WindowLayer::getCursorMode() const {
        return static_cast<CursorMode>(glfwGetInputMode(m_pWindow, GLFW_CURSOR));
    }

    bool WindowLayer::isClosed() const {
        return glfwWindowShouldClose(m_pWindow) == GLFW_TRUE;
    }

    bool WindowLayer::isRunning() const {
        return glfwWindowShouldClose(m_pWindow) == GLFW_FALSE;
    }

    bool WindowLayer::isMaximized() const {
        return glfwGetWindowAttrib(m_pWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
    }

    bool WindowLayer::isMinimized() const {
        return glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED) == GLFW_TRUE;
    }

    bool WindowLayer::isFocused() const {
        return glfwGetWindowAttrib(m_pWindow, GLFW_FOCUSED) == GLFW_TRUE;
    }

    bool WindowLayer::isFullscreen() const {
        return glfwGetWindowMonitor(m_pWindow) != nullptr;
    }

    bool WindowLayer::isResizable() const {
        return glfwGetWindowAttrib(m_pWindow, GLFW_RESIZABLE) == GLFW_TRUE;
    }

    bool WindowLayer::isStickyKeys() const {
        return glfwGetInputMode(m_pWindow, GLFW_STICKY_KEYS) == GLFW_TRUE;
    }

    bool WindowLayer::isLockKeyMods() const {
        return glfwGetInputMode(m_pWindow, GLFW_LOCK_KEY_MODS) == GLFW_TRUE;
    }

    bool WindowLayer::isStickyMouseButtons() const {
        return glfwGetInputMode(m_pWindow, GLFW_STICKY_MOUSE_BUTTONS) == GLFW_TRUE;
    }

    bool WindowLayer::isRawMouseMotion() const {
        return glfwGetInputMode(m_pWindow, GLFW_RAW_MOUSE_MOTION) == GLFW_TRUE;
    }

    bool WindowLayer::isKeyModPressed(const KeyMod::Flags mod) const {
        return (m_KeyMods & mod) > 0;
    }

    bool WindowLayer::isKeyDown(const KeyCode key) const {
        return m_Keys[static_cast<uint32_t>(key)];
    }

    bool WindowLayer::isKeyUp(const KeyCode key) const {
        return !m_Keys[static_cast<uint32_t>(key)];
    }

    bool WindowLayer::isMouseButtonDown(const MouseButton button) const {
        return m_MouseButtons[static_cast<uint32_t>(button)];
    }

    bool WindowLayer::isMouseButtonUp(const MouseButton button) const {
        return !m_MouseButtons[static_cast<uint32_t>(button)];
    }
}  // namespace Ignis