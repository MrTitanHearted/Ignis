#include <Ignis/Engine.hpp>

namespace Ignis {
    Engine *Engine::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Engine::State Engine::s_State{};)

    void Engine::Initialize(Engine *engine, const Settings &settings) {
        DIGNIS_ASSERT(s_pInstance == nullptr, "Ignis::Engine is already initialized");

        s_pInstance = engine;

        SingletonManager::Initialize(&s_pInstance->m_SingletonManager);
        Window::Initialize(&s_pInstance->m_Window, settings.WindowSettings);

        s_pInstance->m_IsRunning.store(false);

        DIGNIS_LOG_ENGINE_INFO("Ignis::Engine Initialized");
    }

    void Engine::Shutdown() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");

        Window::Shutdown();
        SingletonManager::Shutdown();

        DIGNIS_LOG_ENGINE_INFO("Ignis::Engine Shutdown");

        s_pInstance = nullptr;
    }

    void Engine::Stop() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        s_pInstance->m_IsRunning.store(false);
    }

    void Engine::Start() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        s_pInstance->m_IsRunning.store(true);
    }

    bool Engine::IsRunning() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        return s_pInstance->m_IsRunning.load();
    }

    Entity Engine::CreateEntity() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        return Ignis::CreateEntity();
    }

    IGNIS_IF_DEBUG(Engine::State::~State() {
        assert(s_pInstance == nullptr && "Forgot to shutdown Ignis::Engine");
    })
}  // namespace Ignis