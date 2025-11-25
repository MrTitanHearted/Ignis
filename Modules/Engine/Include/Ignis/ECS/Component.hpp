#pragma once

#include <Ignis/Core.hpp>

#include <Ignis/ECS/Entity.hpp>

namespace Ignis {
    typedef uint32_t ComponentHandle;

    static constexpr ComponentHandle INVALID_COMPONENT_HANDLE = 0u;

    class IComponentStore {
       public:
        virtual ~IComponentStore() = default;
    };

    template <typename TComponent>
    class ComponentStore final : public IComponentStore {
       public:
        ComponentStore()           = default;
        ~ComponentStore() override = default;

        TComponent &operator[](size_t index);

        bool contains(ComponentHandle handle) const;

        size_t          getSize() const;
        ComponentHandle getEntity(size_t index) const;

        TComponent &add(ComponentHandle handle);
        TComponent *getComponent(ComponentHandle handle);

        void removeComponent(ComponentHandle handle);

       private:
        std::vector<TComponent>      m_Components;
        std::vector<ComponentHandle> m_Handles;

        gtl::flat_hash_map<ComponentHandle, size_t> m_Lookup;
    };

    template <typename TComponent>
    TComponent &ComponentStore<TComponent>::operator[](const size_t index) {
        DIGNIS_ASSERT(index < m_Components.size());
        return m_Components[index];
    }

    template <typename TComponent>
    bool ComponentStore<TComponent>::contains(const ComponentHandle handle) const {
        return m_Lookup.find(handle) != m_Lookup.end();
    }

    template <typename TComponent>
    size_t ComponentStore<TComponent>::getSize() const {
        return m_Handles.size();
    }

    template <typename TComponent>
    ComponentHandle ComponentStore<TComponent>::getEntity(const size_t index) const {
        DIGNIS_ASSERT(index < m_Handles.size());
        return m_Handles[index];
    }

    template <typename TComponent>
    TComponent &ComponentStore<TComponent>::add(const ComponentHandle handle) {
        DIGNIS_ASSERT(handle != INVALID_ENTITY, "Invalid handle");

        DIGNIS_ASSERT(
            m_Lookup.find(handle) == m_Lookup.end(),
            "Only one instance of a component type is allowed per type");

        DIGNIS_ASSERT(m_Handles.size() == m_Components.size());
        DIGNIS_ASSERT(m_Lookup.size() == m_Components.size());

        m_Lookup[handle]      = m_Components.size();
        TComponent &component = m_Components.emplace_back();

        m_Handles.push_back(handle);

        return component;
    }

    template <typename TComponent>
    TComponent *ComponentStore<TComponent>::getComponent(const ComponentHandle handle) {
        const auto it = m_Lookup.find(handle);
        if (it == m_Lookup.end()) {
            DIGNIS_LOG_ENGINE_WARN("ComponentHandle {} does not exist in this ComponentStore", handle);
            return nullptr;
        }
        return &m_Components[it->second];
    }

    template <typename TComponent>
    void ComponentStore<TComponent>::removeComponent(const ComponentHandle handle) {
        const auto it = m_Lookup.find(handle);
        if (it == m_Lookup.end()) {
            DIGNIS_LOG_ENGINE_WARN("Entity {} does not exist in this ComponentStore", handle);
            return;
        }

        const size_t index = it->second;

        if (index < m_Components.size() - 1) {
            m_Components[index] = std::move(m_Components.back());
            m_Handles[index]    = m_Handles.back();

            m_Lookup[m_Handles[index]] = index;
        }

        m_Components.pop_back();
        m_Handles.pop_back();
        m_Lookup.erase(handle);
    }
}  // namespace Ignis