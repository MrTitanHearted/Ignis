#include <Ignis/Engine.hpp>

namespace Ignis {
    Engine *Engine::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Engine::State Engine::s_State{};)

    Engine &Engine::GetRef() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized");
        return *s_pInstance;
    }

    void Engine::initialize(Settings &settings) {
        DIGNIS_ASSERT(s_pInstance == nullptr, "Ignis::Engine is already initialized");

        m_IsRunning.store(false);

        m_Window.initialize(settings.WindowSettings);
        m_Vulkan.initialize(settings.VulkanSettings);
        m_Frame.initialize(settings.FrameSettings);

        m_GUISystem = std::move(settings.UISystem);
        m_GUISystem->onAttach();

        m_LayerStack.clear();
        m_LayerLookUp.clear();

        s_pInstance = this;
        DIGNIS_LOG_ENGINE_INFO("Ignis::Engine Initialized");
    }

    void Engine::shutdown() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized");

        while (!m_LayerStack.empty()) {
            const std::type_index type = m_LayerStack.back()->m_LayerID;

            m_Window.removeListener(type);
            m_LayerLookUp.erase(type);
            m_LayerStack.pop_back();
        }

        m_Window.removeListener(m_GUISystem->m_ID);
        m_GUISystem->onDetach();
        m_GUISystem = nullptr;

        m_Frame.shutdown();
        m_Vulkan.shutdown();
        m_Window.shutdown();

        DIGNIS_LOG_ENGINE_INFO("Ignis::Engine Shutdown");

        s_pInstance = nullptr;
    }

    void Engine::run() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized");
        m_IsRunning.store(true);
        m_DeltaTime = 0.0f;
        m_Timer.start();
        while (m_IsRunning.load()) {
            m_Timer.stop();
            m_DeltaTime = m_Timer.getElapsedTime();
            m_Timer.start();

            Window::PollEvents();
            m_Window.processEvents();

            for (const auto &layer : m_LayerStack)
                layer->onUpdate(m_DeltaTime);

            m_GUISystem->onGUIBegin();

            for (const auto &layer : m_LayerStack)
                layer->onGUI(m_GUISystem.get());

            m_GUISystem->onGUIEnd();

            if (!m_Frame.begin())
                continue;

            FrameGraph &frame_graph = m_Frame.getFrameGraph();

            for (const auto &layer : m_LayerStack)
                layer->onRender(frame_graph);

            m_GUISystem->onRender(frame_graph);

            if (!m_Frame.end(frame_graph.endFrame())) {
                // continue;
            }
        }
    }

    void Engine::stop() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized");
        m_IsRunning.store(false);
    }

    bool Engine::isRunning() const {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized");
        return m_IsRunning.load();
    }

    FrameGraph &Engine::getFrameGraph() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized");
        return m_Frame.m_FrameGraph;
    }

    IGNIS_IF_DEBUG(Engine::State::~State() {
        assert(s_pInstance == nullptr && "Forgot to shutdown Ignis::Engine");
    })
}  // namespace Ignis