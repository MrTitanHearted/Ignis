#include <Ignis/Engine.hpp>

namespace Ignis {
    Engine *Engine::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Engine::State Engine::s_State{};)

    void Engine::Initialize(Engine *engine, Settings &settings) {
        DIGNIS_ASSERT(s_pInstance == nullptr, "Ignis::Engine is already initialized");

        s_pInstance = engine;
        s_pInstance->m_IsRunning.store(false);

        Window::Initialize(&s_pInstance->m_Window, settings.WindowSettings);
        Vulkan::Initialize(&s_pInstance->m_Vulkan, settings.VulkanSettings);
        Render::Initialize(&s_pInstance->m_Render, settings.RenderSettings);

        s_pInstance->m_UISystem = std::move(settings.UISystem);
        s_pInstance->m_UISystem->initialize();

        s_pInstance->m_LayerStack.clear();
        s_pInstance->m_LayerLookUp.clear();
        s_pInstance->m_LayerTypes.clear();

        DIGNIS_LOG_ENGINE_INFO("Ignis::Engine Initialized");
    }

    void Engine::Shutdown() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");

        while (!s_pInstance->m_LayerStack.empty()) {
            const std::type_index type = s_pInstance->m_LayerTypes.back();
            s_pInstance->m_Window.removeListener(type);
            s_pInstance->m_LayerLookUp.erase(type);
            s_pInstance->m_LayerTypes.pop_back();
            s_pInstance->m_LayerStack.pop_back();
        }

        s_pInstance->m_UISystem->release();
        s_pInstance->m_UISystem = nullptr;

        Render::Shutdown();
        Vulkan::Shutdown();
        Window::Shutdown();

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

            Window::PollEvents();
            m_Window.processEvents();

            for (const auto &layer : m_LayerStack)
                layer->onUpdate(m_DeltaTime);

            m_UISystem->begin();

            for (const auto &layer : m_LayerStack)
                layer->onUI(m_UISystem.get());

            m_UISystem->end();

            std::optional<Render::FrameImage> frame_image_opt = Render::BeginFrame();
            if (!frame_image_opt.has_value()) {
                const auto [width, height] = Window::GetSize();
                Render::Resize(width, height);
                continue;
            }

            const auto &[handle, view, extent] = frame_image_opt.value();

            FrameGraph frame_graph{handle, view, extent};

            for (const auto &layer : m_LayerStack)
                layer->onRender(frame_graph);

            m_UISystem->render(frame_graph);

            if (!Render::EndFrame(frame_graph.build())) {
                const auto [width, height] = Window::GetSize();
                Render::Resize(width, height);
                continue;
            }
        }
    }

    void Engine::stop() {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        m_IsRunning.store(false);
    }

    bool Engine::isRunning() const {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        return m_IsRunning.load();
    }

    IUISystem *Engine::getUISystem() const {
        DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Engine is not initialized");
        return m_UISystem.get();
    }

    IGNIS_IF_DEBUG(Engine::State::~State() {
        assert(s_pInstance == nullptr && "Forgot to shutdown Ignis::Engine");
    })
}  // namespace Ignis