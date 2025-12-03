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
        m_Render.initialize(settings.RenderSettings);

        m_UISystem = std::move(settings.UISystem);
        m_UISystem->initialize();

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

        m_UISystem->release();
        m_UISystem = nullptr;

        m_Render.shutdown();
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

            m_UISystem->begin();

            for (const auto &layer : m_LayerStack)
                layer->onGUI(m_UISystem.get());

            m_UISystem->end();

            if (!m_Render.beginFrame()) {
                continue;
            }

            FrameGraph &frame_graph = m_Render.getFrameGraph();

            for (const auto &layer : m_LayerStack)
                layer->onRender(frame_graph);

            m_UISystem->render(frame_graph);

            if (!m_Render.endFrame(frame_graph.endFrame())) {
                continue;
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

    IGUISystem *Engine::getUISystem() const {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized");
        return m_UISystem.get();
    }

    FrameGraph &Engine::getFrameGraph() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized");
        return m_Render.m_FrameGraph;
    }

    IGNIS_IF_DEBUG(Engine::State::~State() {
        assert(s_pInstance == nullptr && "Forgot to shutdown Ignis::Engine");
    })
}  // namespace Ignis