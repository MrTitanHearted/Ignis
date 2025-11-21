#pragma once

#include <Ignis/Core.hpp>

#include <Ignis/ECS/Entity.hpp>

namespace Ignis {
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

        bool contains(Entity entity) const;

        size_t getSize() const;
        Entity getEntity(size_t index) const;

        TComponent &add(Entity entity);
        TComponent *getComponent(Entity entity);

        void removeComponent(Entity entity);

       private:
        std::vector<TComponent> m_Components;
        std::vector<Entity>     m_Entities;

        gtl::flat_hash_map<Entity, size_t> m_Lookup;
    };

    template <typename TComponent>
    TComponent &ComponentStore<TComponent>::operator[](const size_t index) {
        DIGNIS_ASSERT(index < m_Components.size());
        return m_Components[index];
    }

    template <typename TComponent>
    bool ComponentStore<TComponent>::contains(const Entity entity) const {
        return m_Lookup.find(entity) != m_Lookup.end();
    }

    template <typename TComponent>
    size_t ComponentStore<TComponent>::getSize() const {
        return m_Entities.size();
    }

    template <typename TComponent>
    Entity ComponentStore<TComponent>::getEntity(const size_t index) const {
        DIGNIS_ASSERT(index < m_Entities.size());
        return m_Entities[index];
    }

    template <typename TComponent>
    TComponent &ComponentStore<TComponent>::add(const Entity entity) {
        DIGNIS_ASSERT(entity != INVALID_ENTITY, "Invalid entity");

        DIGNIS_ASSERT(
            m_Lookup.find(entity) == m_Lookup.end(),
            "Only one instance of a component type is allowed per type");

        DIGNIS_ASSERT(m_Entities.size() == m_Components.size());
        DIGNIS_ASSERT(m_Lookup.size() == m_Components.size());

        m_Lookup[entity]      = m_Components.size();
        TComponent &component = m_Components.emplace_back();

        m_Entities.push_back(entity);

        return component;
    }

    template <typename TComponent>
    TComponent *ComponentStore<TComponent>::getComponent(const Entity entity) {
        const auto it = m_Lookup.find(entity);
        if (it == m_Lookup.end()) {
            DIGNIS_LOG_ENGINE_WARN("Entity {} does not exist in this ComponentStore", entity);
            return nullptr;
        }
        return &m_Components[it->second];
    }

    template <typename TComponent>
    void ComponentStore<TComponent>::removeComponent(const Entity entity) {
        const auto it = m_Lookup.find(entity);
        if (it == m_Lookup.end()) {
            DIGNIS_LOG_ENGINE_WARN("Entity {} does not exist in this ComponentStore", entity);
            return;
        }

        const size_t index = it->second;

        if (index < m_Components.size() - 1) {
            m_Components[index] = std::move(m_Components.back());
            m_Entities[index]   = m_Entities.back();

            m_Lookup[m_Entities[index]] = index;
        }

        m_Components.pop_back();
        m_Entities.pop_back();
        m_Lookup.erase(entity);
    }
}  // namespace Ignis