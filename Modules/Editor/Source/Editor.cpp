#include <Ignis/Editor.hpp>

#include <spdlog/sinks/basic_file_sink.h>

namespace Ignis {
    void Editor::initialize(
        const int32_t argc,
        const char  **argv) {
        (void)argc;
        (void)argv;

        Logger::Settings logger_settings{};
        logger_settings.ApplicationLogName = "EDITOR";
#ifdef IGNIS_BUILD_TYPE_DEBUG
        logger_settings.EngineLogLevel      = spdlog::level::trace;
        logger_settings.ApplicationLogLevel = spdlog::level::trace;
#else
        logger_settings.EngineLogLevel      = spdlog::level::err;
        logger_settings.ApplicationLogLevel = spdlog::level::warn;
#endif

        const spdlog::sink_ptr editor_sink =
            logger_settings.ApplicationSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                "IGNIS_EDITOR.log", true));
        editor_sink->set_pattern(Logger::GetFileSinkPattern());

        Logger::Initialize(&m_Logger, logger_settings);
        Engine::Initialize(
            &m_Engine,
            {
                .WindowSettings = WindowLayer::Settings{
                    .Title = "Ignis::Editor",
                },
            });

        m_Engine.getLayer<WindowLayer>()->setIcon("Assets/Icons/IgnisEditor.png");

        WindowLayer *window_layer = m_Engine.getLayer<WindowLayer>();
        VulkanLayer *vulkan_layer = m_Engine.getLayer<VulkanLayer>();
        RenderLayer *render_layer = m_Engine.getLayer<RenderLayer>();

        m_Engine.pushLayer<ImGuiLayer>(
            window_layer,
            vulkan_layer,
            render_layer,
            ImGuiLayer::Settings{});

        ImGuiLayer *im_gui_layer = m_Engine.getLayer<ImGuiLayer>();

        m_Engine.pushLayer<EditorLayer>(
            vulkan_layer,
            render_layer,
            im_gui_layer,
            EditorLayer::Settings{});

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor initialized");
    }

    void Editor::release() {
        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor released");

        m_Engine.stop();

        Engine::Shutdown();
        Logger::Shutdown();
    }

    void Editor::run() {
        m_Engine.run();
    }
}  // namespace Ignis