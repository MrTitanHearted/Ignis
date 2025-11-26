#include <Ignis/Engine/WindowLayer/Enums.hpp>

namespace Ignis {
    std::string_view GetKeyCodeName(const KeyCode key) {
        static std::string key_name{};
        key_name.clear();

        key_name = glfwGetKeyName(static_cast<uint32_t>(key), 0);

        if (!key_name.empty())
            return key_name;

        switch (key) {
            case KeyCode::eEscape:
                return "Escape";
            case KeyCode::eDelete:
                return "Delete";
            case KeyCode::eLeftAlt:
                return "Left Alt";
            case KeyCode::eRightAlt:
                return "Right Alt";
            case KeyCode::eLeftControl:
                return "Left Ctrl";
            case KeyCode::eRightControl:
                return "Right Ctrl";
            case KeyCode::eLeftShift:
                return "Left Shift";
            case KeyCode::eRightShift:
                return "Right Shift";
            case KeyCode::eLeftSuper:
                return "Left Super";
            case KeyCode::eRightSuper:
                return "Right Super";
            case KeyCode::eCapsLock:
                return "Caps Lock";
            case KeyCode::eTab:
                return "Tab";
            case KeyCode::eBackspace:
                return "Backspace";
            case KeyCode::eLeft:
                return "Left Arrow";
            case KeyCode::eDown:
                return "Down Arrow";
            case KeyCode::eUp:
                return "Up Arrow";
            case KeyCode::eRight:
                return "Right Arrow";
            case KeyCode::eNumLock:
                return "Num Lock";
            case KeyCode::ePrintScreen:
                return "Print Screen";
            case KeyCode::eScrollLock:
                return "Scroll Lock";
            case KeyCode::ePause:
                return "Pause";
            case KeyCode::eHome:
                return "Home";
            case KeyCode::eEnd:
                return "End";
            case KeyCode::ePageUp:
                return "Page Up";
            case KeyCode::ePageDown:
                return "Page Down";
            case KeyCode::eSpace:
                return "Space";
            case KeyCode::eEnter:
                return "Enter";
            case KeyCode::eKpEnter:
                return "Keypad Enter";

            default:
                return "Unknown Key";
        }
    }

    std::string_view GetKeyModFlagsName(const KeyMod::Flags mods) {
        if (mods == KeyMod::eNone)
            return "None";

        static std::string name{};
        name.clear();

        if ((mods & KeyMod::eShiftBit) > 0) {
            name += "Shift";
        }

        if ((mods & KeyMod::eControlBit) > 0) {
            if (!name.empty())
                name += " | ";
            name += "Control";
        }

        if ((mods & KeyMod::eAltBit) > 0) {
            if (!name.empty())
                name += " | ";
            name += "Alt";
        }

        if ((mods & KeyMod::eSuperBit) > 0) {
            if (!name.empty())
                name += " | ";
            name += "Super";
        }

        if ((mods & KeyMod::eCapsLockBit) > 0) {
            if (!name.empty())
                name += " | ";
            name += "CapsLock";
        }

        if ((mods & KeyMod::eNumLockBit) > 0) {
            if (!name.empty())
                name += " | ";
            name += "NumLock";
        }

        return name;
    }

    std::string_view GetKeyActionName(const KeyAction action) {
        static std::string action_name{};
        action_name.clear();

        switch (action) {
            case KeyAction::ePress:
                action_name = "Press";
                break;
            case KeyAction::eRelease:
                action_name = "Release";
                break;
            case KeyAction::eRepeat:
                action_name = "Repeat";
                break;
        }

        return action_name;
    }

    std::string_view GetMouseButtonName(const MouseButton button) {
        static std::string button_name{};
        button_name.clear();

        switch (button) {
            case MouseButton::eLeft:
                button_name = "Left";
                break;
            case MouseButton::eRight:
                button_name = "Right";
                break;
            case MouseButton::eMiddle:
                button_name = "Middle";
                break;
            default:
                const uint32_t button_index = static_cast<uint32_t>(button);
                button_name                 = std::vformat("X{}", std::make_format_args(button_index));
        }

        return button_name;
    }

    std::string_view GetCursorModeName(const CursorMode mode) {
        static std::string mode_name{};
        mode_name.clear();

        switch (mode) {
            case CursorMode::eNormal:
                mode_name = "Normal";
                break;
            case CursorMode::eHidden:
                mode_name = "Hidden";
                break;
            case CursorMode::eDisabled:
                mode_name = "Disabled";
                break;
            case CursorMode::eCaptured:
                mode_name = "Captured";
                break;
        }

        return mode_name;
    }
}  // namespace Ignis