#include <Ignis/Core/Singleton.hpp>

namespace Ignis {
    SingletonManager *SingletonManager::s_pInstance = nullptr;

    void SingletonManager::Initialize(SingletonManager *p_singleton_manager) {
        DIGNIS_ASSERT(s_pInstance == nullptr, "Ignis::SingletonManager is already initialized");

        s_pInstance = p_singleton_manager;

        DIGNIS_LOG_ENGINE_INFO("Ignis::SingletonManager Initialized");
    }

    void SingletonManager::Shutdown() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::SingletonManager is not initialized");

        s_pInstance->m_Singletons.clear();
        s_pInstance->m_Types.clear();
        s_pInstance->m_Lookup.clear();

        s_pInstance = nullptr;

        DIGNIS_LOG_ENGINE_INFO("Ignis::SingletonManager Shutdown");
    }
}  // namespace Ignis