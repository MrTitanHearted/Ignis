#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    inline IGUISystem::IGUISystem(const std::type_index type) : m_ID{type} {}

    template <typename TEvent, typename Callback>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void IGUISystem::attachCallback(Callback callback) const {
        Engine::GetRef().m_Window.addListener(callback, m_ID);
    }

    template <typename TEvent>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void IGUISystem::detachCallback() const {
        Engine::GetRef().m_Window.removeListener(m_ID);
    }

    template <typename TEvent, typename... Args>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void IGUISystem::postEvent(Args &&...args) const {
        Engine::GetRef().m_Window.postEvent(m_ID, std::forward<Args>(args)...);
    }

    template <typename TGUISystem>
    GUISystem<TGUISystem>::GUISystem() : IGUISystem{typeid(TGUISystem)} {}
}  // namespace Ignis