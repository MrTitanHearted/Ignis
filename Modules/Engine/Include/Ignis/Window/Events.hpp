#pragma once

#include <Ignis/Window/Enums.hpp>

namespace Ignis {
    class WindowCloseEvent {};

    class WindowResizeEvent {
       public:
        uint32_t Width;
        uint32_t Height;

        WindowResizeEvent(
            const uint32_t width,
            const uint32_t height)
            : Width(width), Height(height) {}
    };

    class WindowContentScaleEvent {
       public:
        float XScale;
        float YScale;

        WindowContentScaleEvent(
            const float x_scale,
            const float y_scale)
            : XScale(x_scale), YScale(y_scale) {}
    };

    class WindowMoveEvent {
       public:
        uint32_t X;
        uint32_t Y;

        WindowMoveEvent(const uint32_t x, const uint32_t y)
            : X(x), Y(y) {}
    };

    class WindowRestoreEvent {};

    class WindowMinimizeEvent {};

    class WindowMaximizeEvent {};

    class WindowGainFocusEvent {};

    class WindowLoseFocusEvent {};

    class WindowRefreshEvent {};

    class WindowKeyEvent {
       public:
        KeyCode       Key;
        KeyMod::Flags ModFlags;
        KeyAction     Action;

        WindowKeyEvent(
            const KeyCode       key,
            const KeyMod::Flags mod_flags,
            const KeyAction     action)
            : Key(key), ModFlags(mod_flags), Action(action) {}
    };

    class WindowCharEvent {
       public:
        uint32_t Codepoint;

        explicit WindowCharEvent(const uint32_t codepoint)
            : Codepoint(codepoint) {}
    };

    class WindowMouseMoveEvent {
       public:
        double X;
        double Y;

        WindowMouseMoveEvent(
            const double x,
            const double y)
            : X(x), Y(y) {
        }
    };

    class WindowMouseEnterEvent {};

    class WindowMouseLeaveEvent {};

    class WindowMouseButtonEvent {
       public:
        MouseButton   Button;
        KeyMod::Flags ModFlags;
        KeyAction     Action;

        WindowMouseButtonEvent(
            const MouseButton   button,
            const KeyMod::Flags mod_flags,
            const KeyAction     action)
            : Button(button), ModFlags(mod_flags), Action(action) {}
    };

    class WindowMouseScrollEvent {
       public:
        double XOffset;
        double YOffset;

        WindowMouseScrollEvent(
            const double x,
            const double y)
            : XOffset(x), YOffset(y) {}
    };

    class WindowDropEvent {
       public:
        std::vector<std::string> Paths;

        explicit WindowDropEvent(std::vector<std::string> &&paths) {
            Paths = std::move(paths);
        }
    };

    typedef std::variant<
        WindowCloseEvent,
        WindowResizeEvent,
        WindowContentScaleEvent,
        WindowMoveEvent,
        WindowRestoreEvent,
        WindowMinimizeEvent,
        WindowMaximizeEvent,
        WindowGainFocusEvent,
        WindowLoseFocusEvent,
        WindowRefreshEvent,
        WindowKeyEvent,
        WindowCharEvent,
        WindowMouseMoveEvent,
        WindowMouseEnterEvent,
        WindowMouseLeaveEvent,
        WindowMouseButtonEvent,
        WindowMouseScrollEvent,
        WindowDropEvent>
        WindowEvent;

    template <class... WindowEvents>
    struct WindowEventVisitor : WindowEvents... {
        using WindowEvents::operator()...;
    };

    template <class... WindowEvents>
    WindowEventVisitor(WindowEvents...) -> WindowEventVisitor<WindowEvents...>;

    class WindowEvents {
       public:
        template <typename... Handlers>
        void visit(Handlers &&...handlers) {
            auto visitor = WindowEventVisitor{std::forward<Handlers>(handlers)..., [](auto const &) {}};
            for (const WindowEvent &event : m_Events) {
                std::visit(visitor, event);
            }
        }

       public:
        WindowEvents()  = default;
        ~WindowEvents() = default;

        void clear();

        std::span<const WindowEvent> getEvents() const;

        std::vector<WindowEvent>::const_iterator begin() const;
        std::vector<WindowEvent>::const_iterator end() const;

       private:
        void addCloseEvent(WindowCloseEvent &&event);
        void addResizeEvent(WindowResizeEvent &&event);
        void addContentScaleEvent(WindowContentScaleEvent &&event);
        void addMoveEvent(WindowMoveEvent &&event);
        void addRestoreEvent(WindowRestoreEvent &&event);
        void addMinimizeEvent(WindowMinimizeEvent &&event);
        void addMaximizeEvent(WindowMaximizeEvent &&event);
        void addGainFocusEvent(WindowGainFocusEvent &&event);
        void addLoseFocusEvent(WindowLoseFocusEvent &&event);
        void addRefreshEvent(WindowRefreshEvent &&event);
        void addKeyEvent(WindowKeyEvent &&event);
        void addCharEvent(WindowCharEvent &&event);
        void addMouseMoveEvent(WindowMouseMoveEvent &&event);
        void addMouseEnterEvent(WindowMouseEnterEvent &&event);
        void addMouseLeaveEvent(WindowMouseLeaveEvent &&event);
        void addMouseButtonEvent(WindowMouseButtonEvent &&event);
        void addMouseScrollEvent(WindowMouseScrollEvent &&event);
        void addDropEvent(WindowDropEvent &&event);

       private:
        std::vector<WindowEvent> m_Events;

       private:
        friend class Window;
    };
}  // namespace Ignis