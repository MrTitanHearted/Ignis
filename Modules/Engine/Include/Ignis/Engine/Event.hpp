#pragma once

#include <Ignis/Core.hpp>

namespace Ignis {
#define IGNIS_EVENT_CLASS_TYPE(type)                    \
    static EventType GetType() { return typeid(type); } \
                                                        \
    EventType getType() const override { return typeid(type); }

    typedef std::type_index EventType;

    class AEvent {
       public:
        virtual ~AEvent() = default;

        virtual EventType getType() const = 0;

        void Handle(const bool is_handled) {
            m_IsHandled = is_handled;
        }

        bool IsHandled() const {
            return m_IsHandled;
        }

       private:
        bool m_IsHandled = false;
    };

    class EventDispatcher {
       public:
        template <typename TEvent, typename = std::enable_if_t<std::is_base_of_v<AEvent, TEvent>>>
        using EventFn = fu2::function<bool(const TEvent &)>;

       public:
        explicit EventDispatcher(AEvent &event) : m_Event(event) {}

        ~EventDispatcher() = default;

        template <typename TEvent, typename = std::enable_if_t<std::is_base_of_v<AEvent, TEvent>>>
        bool dispatch(EventFn<TEvent> fn) {
            if (const EventType type = typeid(TEvent); type != m_Event.getType()) {
                return false;
            }

            m_Event.Handle(fn(*static_cast<const TEvent *>(&m_Event)));
            return true;
        }

        template <typename TEvent, typename TClass, typename = std::enable_if_t<std::is_base_of_v<AEvent, TEvent>>>
        bool dispatch(TClass *object, bool (TClass::*method)(const TEvent &event)) {
            if (const EventType type = typeid(TEvent); type != m_Event.getType()) {
                return false;
            }

            m_Event.Handle((object->*method)(*static_cast<const TEvent *>(&m_Event)));
            return true;
        }

        template <typename TEvent, typename TClass, typename = std::enable_if_t<std::is_base_of_v<AEvent, TEvent>>>
        bool dispatch(const TClass *object, bool (TClass::*method)(const TEvent &event) const) {
            if (const EventType type = typeid(TEvent); type != m_Event.getType()) {
                return false;
            }

            m_Event.Handle((object->*method)(*static_cast<const TEvent *>(&m_Event)));
            return true;
        }

       private:
        AEvent &m_Event;
    };
}  // namespace Ignis