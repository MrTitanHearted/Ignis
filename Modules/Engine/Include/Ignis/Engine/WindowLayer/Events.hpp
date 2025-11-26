#pragma once

#include <Ignis/Engine/Event.hpp>

#include <Ignis/Engine/WindowLayer/Enums.hpp>

namespace Ignis {
    class WindowCloseEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowCloseEvent);
    };

    class WindowResizeEvent final : public AEvent {
       public:
        WindowResizeEvent(
            const uint32_t width,
            const uint32_t height)
            : Width(width), Height(height) {}

        IGNIS_EVENT_CLASS_TYPE(WindowResizeEvent);

       public:
        uint32_t Width;
        uint32_t Height;
    };

    class WindowContentScaleEvent final : public AEvent {
       public:
        WindowContentScaleEvent(
            const float x_scale,
            const float y_scale)
            : XScale(x_scale), YScale(y_scale) {}

        IGNIS_EVENT_CLASS_TYPE(WindowContentScaleEvent);

       public:
        float XScale;
        float YScale;
    };

    class WindowMoveEvent final : public AEvent {
       public:
        WindowMoveEvent(const uint32_t x, const uint32_t y)
            : X(x), Y(y) {}

        IGNIS_EVENT_CLASS_TYPE(WindowMoveEvent);

       public:
        uint32_t X;
        uint32_t Y;
    };

    class WindowRestoreEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowRestoreEvent);
    };

    class WindowMinimizeEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowMinimizeEvent);
    };

    class WindowMaximizeEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowMaximizeEvent);
    };

    class WindowGainFocusEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowGainFocusEvent);
    };

    class WindowLoseFocusEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowLoseFocusEvent);
    };

    class WindowRefreshEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowRefreshEvent);
    };

    class WindowKeyEvent final : public AEvent {
       public:
        WindowKeyEvent(
            const KeyCode       key,
            const KeyMod::Flags mod_flags,
            const KeyAction     action)
            : Key(key), ModFlags(mod_flags), Action(action) {}

        IGNIS_EVENT_CLASS_TYPE(WindowKeyEvent);

       public:
        KeyCode       Key;
        KeyMod::Flags ModFlags;
        KeyAction     Action;
    };

    class WindowCharEvent final : public AEvent {
       public:
        explicit WindowCharEvent(const uint32_t codepoint)
            : Codepoint(codepoint) {}

        IGNIS_EVENT_CLASS_TYPE(WindowCharEvent);

       public:
        uint32_t Codepoint;
    };

    class WindowMouseMoveEvent final : public AEvent {
       public:
        WindowMouseMoveEvent(
            const double x,
            const double y)
            : X(x), Y(y) {
        }

        IGNIS_EVENT_CLASS_TYPE(WindowMouseMoveEvent);

       public:
        double X;
        double Y;
    };

    class WindowMouseEnterEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowMouseEnterEvent);
    };

    class WindowMouseLeaveEvent final : public AEvent {
       public:
        IGNIS_EVENT_CLASS_TYPE(WindowMouseLeaveEvent);
    };

    class WindowMouseButtonEvent final : public AEvent {
       public:
        WindowMouseButtonEvent(
            const MouseButton   button,
            const KeyMod::Flags mod_flags,
            const KeyAction     action)
            : Button(button), ModFlags(mod_flags), Action(action) {}

        IGNIS_EVENT_CLASS_TYPE(WindowMouseButtonEvent);

       public:
        MouseButton   Button;
        KeyMod::Flags ModFlags;
        KeyAction     Action;
    };

    class WindowMouseScrollEvent final : public AEvent {
       public:
        WindowMouseScrollEvent(
            const double x,
            const double y)
            : XOffset(x), YOffset(y) {}

        IGNIS_EVENT_CLASS_TYPE(WindowMouseScrollEvent);

       public:
        double XOffset;
        double YOffset;
    };

    class WindowDropEvent final : public AEvent {
       public:
        explicit WindowDropEvent(std::vector<std::string> &&paths) {
            Paths = std::move(paths);
        }

        IGNIS_EVENT_CLASS_TYPE(WindowDropEvent);

       public:
        std::vector<std::string> Paths;
    };
}  // namespace Ignis