#pragma once

#include <Ignis/Core.hpp>

#include <Ignis/Window/Enums.hpp>

namespace Ignis {
    class WindowCloseEvent final : public IEvent {
       public:
    };

    class WindowResizeEvent final : public IEvent {
       public:
        WindowResizeEvent(
            const uint32_t width,
            const uint32_t height)
            : Width(width), Height(height) {}

       public:
        uint32_t Width;
        uint32_t Height;
    };

    class WindowContentScaleEvent final : public IEvent {
       public:
        WindowContentScaleEvent(
            const float x_scale,
            const float y_scale)
            : XScale(x_scale), YScale(y_scale) {}

       public:
        float XScale;
        float YScale;
    };

    class WindowMoveEvent final : public IEvent {
       public:
        WindowMoveEvent(const uint32_t x, const uint32_t y)
            : X(x), Y(y) {}

       public:
        uint32_t X;
        uint32_t Y;
    };

    class WindowRestoreEvent final : public IEvent {
       public:
    };

    class WindowMinimizeEvent final : public IEvent {
       public:
    };

    class WindowMaximizeEvent final : public IEvent {
       public:
    };

    class WindowGainFocusEvent final : public IEvent {
       public:
    };

    class WindowLoseFocusEvent final : public IEvent {
       public:
    };

    class WindowRefreshEvent final : public IEvent {
       public:
    };

    class WindowKeyEvent final : public IEvent {
       public:
        WindowKeyEvent(
            const KeyCode       key,
            const KeyMod::Flags mod_flags,
            const KeyAction     action)
            : Key(key), ModFlags(mod_flags), Action(action) {}

       public:
        KeyCode       Key;
        KeyMod::Flags ModFlags;
        KeyAction     Action;
    };

    class WindowCharEvent final : public IEvent {
       public:
        explicit WindowCharEvent(const uint32_t codepoint)
            : Codepoint(codepoint) {}

       public:
        uint32_t Codepoint;
    };

    class WindowMouseMoveEvent final : public IEvent {
       public:
        WindowMouseMoveEvent(
            const double x,
            const double y)
            : X(x), Y(y) {
        }

       public:
        double X;
        double Y;
    };

    class WindowMouseEnterEvent final : public IEvent {
       public:
    };

    class WindowMouseLeaveEvent final : public IEvent {
       public:
    };

    class WindowMouseButtonEvent final : public IEvent {
       public:
        WindowMouseButtonEvent(
            const MouseButton   button,
            const KeyMod::Flags mod_flags,
            const KeyAction     action)
            : Button(button), ModFlags(mod_flags), Action(action) {}

       public:
        MouseButton   Button;
        KeyMod::Flags ModFlags;
        KeyAction     Action;
    };

    class WindowMouseScrollEvent final : public IEvent {
       public:
        WindowMouseScrollEvent(
            const double x,
            const double y)
            : XOffset(x), YOffset(y) {}

       public:
        double XOffset;
        double YOffset;
    };

    class WindowDropEvent final : public IEvent {
       public:
        explicit WindowDropEvent(std::vector<std::string> &&paths) {
            Paths = std::move(paths);
        }

       public:
        std::vector<std::string> Paths;
    };
}  // namespace Ignis