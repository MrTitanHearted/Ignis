#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    inline AGUISystem::AGUISystem(const std::type_index type) : m_ID{type} {}

    template <typename TEvent, typename Callback>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void AGUISystem::attachCallback(Callback callback) const {
        Engine::GetRef().m_Window.addListener(callback, m_ID);
    }

    template <typename TEvent>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void AGUISystem::detachCallback() const {
        Engine::GetRef().m_Window.removeListener(m_ID);
    }

    template <typename TEvent, typename... Args>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void AGUISystem::postEvent(Args &&...args) const {
        Engine::GetRef().m_Window.postEvent(m_ID, std::forward<Args>(args)...);
    }

    template <typename TGUISystem>
    IGUISystem<TGUISystem>::IGUISystem() : AGUISystem{typeid(TGUISystem)} {}
}  // namespace Ignis