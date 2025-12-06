#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    inline ILayer::ILayer(Engine *engine, const std::type_index layer_id)
        : m_pEngine{engine}, m_LayerID{layer_id} {}

    template <typename TEvent, typename Callback>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void ILayer::attachCallback(Callback callback) const {
        m_pEngine->m_Window.addListener<TEvent>(callback, m_LayerID);
    }

    template <typename TEvent>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void ILayer::detachCallback() const {
        m_pEngine->m_Window.removeListener<TEvent>(m_LayerID);
    }

    template <typename TEvent, typename... Args>
        requires(std::is_base_of_v<IEvent, TEvent>)
    void ILayer::postEvent(Args &&...args) const {
        m_pEngine->m_Window.postEvent<TEvent>(std::forward<Args>(args)...);
    }

    inline void ILayer::postTask(fu2::function<void()> &&task) const {
        m_pEngine->m_Window.postTask(std::move(task));
    }

    template <typename TLayer>
    Layer<TLayer>::Layer()
        : ILayer{&Engine::GetRef(), typeid(TLayer)} {}

}  // namespace Ignis