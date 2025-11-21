#pragma once

#include <Ignis/Core.hpp>

namespace Ignis {
    class ISingleton {
       public:
        virtual ~ISingleton() = default;
    };

    class SingletonManager {
       public:
        static void Initialize(SingletonManager *p_singleton_manager);
        static void Shutdown();

        template <typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<ISingleton, T>>>
        static void Set(Args &&...args) {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::SingletonManager is not initialized");

            const std::type_index type_index = typeid(T);

            DIGNIS_ASSERT(s_pInstance->m_Lookup.find(type_index) == s_pInstance->m_Lookup.end(), "Singleton already exists");

            s_pInstance->m_Lookup[type_index] = s_pInstance->m_Singletons.size();
            s_pInstance->m_Types.push_back(type_index);
            s_pInstance->m_Singletons.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<ISingleton, T>>>
        static void Remove() {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::SingletonManager is not initialized");

            const std::type_index type_index = typeid(T);

            const auto it = s_pInstance->m_Lookup.find(type_index);

            DIGNIS_ASSERT(it != s_pInstance->m_Lookup.end(), "Singleton does not exist");

            if (const size_t index = it->second; index < s_pInstance->m_Singletons.size() - 1) {
                s_pInstance->m_Singletons[index] = std::move(s_pInstance->m_Singletons.back());
                s_pInstance->m_Types[index]      = s_pInstance->m_Types.back();

                s_pInstance->m_Lookup[s_pInstance->m_Types[index]] = index;
            }

            s_pInstance->m_Singletons.pop_back();
            s_pInstance->m_Types.pop_back();
            s_pInstance->m_Lookup.erase(it);
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<ISingleton, T>>>
        static bool Exists() {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::SingletonManager is not initialized");

            const std::type_index type_index = typeid(T);

            return s_pInstance->m_Lookup.find(type_index) != s_pInstance->m_Lookup.end();
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<ISingleton, T>>>
        static T &Get() {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::SingletonManager is not initialized");

            const std::type_index type_index = typeid(T);

            const auto it = s_pInstance->m_Lookup.find(type_index);

            DIGNIS_ASSERT(it != s_pInstance->m_Lookup.end(), "Singleton does not exist");

            return *static_cast<T *>(s_pInstance->m_Singletons[it->second].get());
        }

       public:
        SingletonManager()  = default;
        ~SingletonManager() = default;

       private:
        std::vector<std::unique_ptr<ISingleton>> m_Singletons;

        std::vector<std::type_index> m_Types;

        gtl::flat_hash_map<std::type_index, size_t> m_Lookup;

       private:
        static SingletonManager *s_pInstance;
    };
}  // namespace Ignis