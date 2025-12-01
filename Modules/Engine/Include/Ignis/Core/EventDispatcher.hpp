#pragma once

#include <Ignis/Core/Logger.hpp>

namespace Ignis {
    class IEvent {
       public:
        virtual ~IEvent() = default;
    };

    template <typename TEvent>
        requires(std::is_base_of_v<IEvent, TEvent>)
    class EventListener {
       public:
        EventListener(
            fu2::function<bool(const TEvent &)> callback,
            const std::type_index               listener_id)
            : m_Callback{std::move(callback)},
              m_ListenerID{listener_id} {}

        ~EventListener() = default;

        bool operator()(const TEvent &event) {
            return m_Callback(event);
        }

        std::type_index getListenerID() const {
            return m_ListenerID;
        }

       private:
        fu2::function<bool(const TEvent &)> m_Callback;

        std::type_index m_ListenerID;
    };

    class IEventRegistry {
       public:
        virtual ~IEventRegistry() = default;

        virtual void processEvents() = 0;

        virtual void removeListener(std::type_index listener_id) = 0;
    };

    template <typename TEvent>
        requires(std::is_base_of_v<IEvent, TEvent>)
    class EventRegistry final : public IEventRegistry {
       public:
        EventRegistry()           = default;
        ~EventRegistry() override = default;

        void processEvents() override {
            while (!m_EventQueue.empty()) {
                const TEvent event = m_EventQueue.front();
                m_EventQueue.pop();

                bool is_handled = false;
                for (int32_t i = m_Listeners.size() - 1; i >= 0 && !is_handled; --i) {
                    is_handled = m_Listeners[i](event);
                }
            }
        }

        void addListener(
            fu2::function<bool(const TEvent &)> callback,
            const std::type_index               listener_id) {
            const std::type_index event_type = typeid(TEvent);

            DIGNIS_ASSERT(
                !m_ListenerLookUp.contains(listener_id),
                "Ignis::EventRegistry<{}>: Listener '{}' already registered.",
                event_type.name(),
                listener_id.name());

            m_Listeners.emplace_back(
                std::move(callback),
                listener_id);

            m_ListenerLookUp[listener_id] = m_Listeners.size() - 1;
        }

        void removeListener(const std::type_index listener_id) override {
            const std::type_index event_type = typeid(TEvent);

            const auto it = m_ListenerLookUp.find(listener_id);
            DIGNIS_ASSERT(
                it != m_ListenerLookUp.end(),
                "Ignis::EventRegistry<{}>: Listener '{}' not registered.",
                event_type.name(),
                listener_id.name());

            const size_t index = it->second;

            m_Listeners[index] = std::move(m_Listeners.back());

            m_ListenerLookUp[m_Listeners[index].getListenerID()] = index;

            m_Listeners.pop_back();
            m_ListenerLookUp.erase(it);
        }

        void postEvent(TEvent &&event) {
            m_EventQueue.push(std::move(event));
        }

       private:
        gtl::vector<EventListener<TEvent>> m_Listeners;

        gtl::flat_hash_map<std::type_index, size_t> m_ListenerLookUp;

        std::queue<TEvent> m_EventQueue;
    };

    class EventDispatcher {
       public:
        EventDispatcher()          = default;
        virtual ~EventDispatcher() = default;

        template <typename TEvent>
            requires(std::is_base_of_v<IEvent, TEvent>)
        void addListener(
            fu2::function<bool(const TEvent &)> callback,
            const std::type_index               listener_id) {
            getRegistry<TEvent>().addListener(std::move(callback), listener_id);
            m_ListenerEventTypes[listener_id].insert(typeid(TEvent));
        }

        template <typename TEvent>
            requires(std::is_base_of_v<IEvent, TEvent>)
        void removeListener(const std::type_index listener_id) {
            getRegistry<TEvent>().removeListener(listener_id);
            m_ListenerEventTypes[listener_id].erase(typeid(TEvent));
        }

        void removeListener(const std::type_index listener_id) {
            DIGNIS_ASSERT(
                m_ListenerEventTypes.contains(listener_id),
                "Ignis::EventDispatcher::removeListener: Listener '{}' is not added.",
                listener_id.name());
            for (const auto &event_type : m_ListenerEventTypes[listener_id]) {
                m_Registries[event_type]->removeListener(listener_id);
            }
            m_ListenerEventTypes.erase(listener_id);
        }

        template <typename TEvent, typename... Args>
            requires(std::is_base_of_v<IEvent, TEvent>)
        void postEvent(Args &&...args) {
            getRegistry<TEvent>().postEvent(TEvent(std::forward<Args>(args)...));
        }

        void postTask(fu2::function<void()> &&task) {
            m_TaskQueue.push(std::move(task));
        }

        void processEvents() {
            for (const std::unique_ptr<IEventRegistry> &registry : std::views::values(m_Registries)) {
                registry->processEvents();
            }

            while (!m_TaskQueue.empty()) {
                auto task = m_TaskQueue.front();
                m_TaskQueue.pop();
                task();
            }
        }

       private:
        template <typename TEvent>
            requires(std::is_base_of_v<IEvent, TEvent>)
        EventRegistry<TEvent> &getRegistry() {
            const std::type_index event_type = typeid(TEvent);

            if (const auto it = m_Registries.find(event_type);
                it != m_Registries.end()) {
                return *static_cast<EventRegistry<TEvent> *>(it->second.get());
            }

            m_Registries[event_type] = std::make_unique<EventRegistry<TEvent>>();

            return *static_cast<EventRegistry<TEvent> *>(m_Registries[event_type].get());
        }

       private:
        gtl::flat_hash_map<std::type_index, std::unique_ptr<IEventRegistry>> m_Registries;

        gtl::flat_hash_map<std::type_index, gtl::flat_hash_set<std::type_index>> m_ListenerEventTypes;

        std::queue<fu2::function<void()>> m_TaskQueue;
    };
}  // namespace Ignis