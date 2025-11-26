#include <Ignis/Engine.hpp>

namespace Ignis {
    Engine *Engine::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Engine::State Engine::s_State{};)

    void Engine::Initialize(Engine *engine, const Settings &settings) {
        DIGNIS_ASSERT(s_pInstance == nullptr, "Ignis::Engine is already initialized");

        s_pInstance = engine;
        s_pInstance->m_IsRunning.store(false);

        s_pInstance->m_LayerStack.clear();
        s_pInstance->m_LayerLookUp.clear();
        s_pInstance->m_LayerTypes.clear();

        s_pInstance->pushLayer<WindowLayer>(settings.WindowSettings);
        s_pInstance->pushLayer<VulkanLayer>(s_pInstance->getLayer<WindowLayer>(), settings.VulkanSettings);
        s_pInstance->pushLayer<RenderLayer>(s_pInstance->getLayer<VulkanLayer>(), settings.RenderSettings);

        DIGNIS_LOG_ENGINE_INFO("Ignis::Engine Initialized");
    }

    void Engine::Shutdown() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");

        while (!s_pInstance->m_LayerStack.empty()) {
            s_pInstance->m_LayerStack.pop_back();
            const std::type_index type = s_pInstance->m_LayerTypes.back();
            s_pInstance->m_LayerTypes.pop_back();
            s_pInstance->m_LayerLookUp.erase(type);
        }

        DIGNIS_LOG_ENGINE_INFO("Ignis::Engine Shutdown");

        s_pInstance = nullptr;
    }

    Engine &Engine::Get() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        return *s_pInstance;
    }

    void Engine::run() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        m_IsRunning.store(true);
        m_DeltaTime = 0.0f;
        m_Timer.start();
        while (m_IsRunning.load()) {
            m_Timer.stop();
            m_DeltaTime = m_Timer.getElapsedTime();
            m_Timer.start();

            WindowLayer::PollEvents();

            for (const auto &ilayer : m_LayerStack)
                ilayer->onPreUpdate();
            for (const auto &ilayer : m_LayerStack)
                ilayer->onUpdate(m_DeltaTime);
            for (const auto &ilayer : m_LayerStack)
                ilayer->onPostUpdate();

            for (const auto &ilayer : m_LayerStack)
                ilayer->onPreRender();
            for (const auto &ilayer : m_LayerStack)
                ilayer->onRender();
            for (const auto &ilayer : m_LayerStack)
                ilayer->onPostRender();
        }
    }

    void Engine::stop() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        m_IsRunning.store(false);
    }

    void Engine::raiseEvent(AEvent &event) {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        for (const auto &layer : std::views::reverse(s_pInstance->m_LayerStack)) {
            layer->onEvent(event);
            if (event.IsHandled())
                break;
        }
    }

    bool Engine::isRunning() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        return s_pInstance->m_IsRunning.load();
    }

    IGNIS_IF_DEBUG(Engine::State::~State() {
        assert(s_pInstance == nullptr && "Forgot to shutdown Ignis::Engine");
    })
}  // namespace Ignis