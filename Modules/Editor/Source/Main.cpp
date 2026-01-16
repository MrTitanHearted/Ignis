#include <Ignis/Editor.hpp>

#include <Ignis/ImGuiSystem.hpp>

#include <spdlog/sinks/basic_file_sink.h>

int32_t main(
    const int32_t argc,
    const char  **argv) {
    std::vector<std::string_view> arguments{};
    arguments.resize(argc);

    for (uint32_t i = 0; i < argc; i++) {
        arguments[i] = argv[i];
    }

    Ignis::Logger::Settings logger_settings{};
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
    editor_sink->set_pattern(Ignis::Logger::GetFileSinkPattern());

    Ignis::Engine::Settings engine_settings{};

    engine_settings.WindowSettings.Title = "Ignis::Editor";
    engine_settings.WindowSettings.Icon  = "Assets/Icons/IgnisEditor.png";

    engine_settings.RenderSettings.SkyboxFacePaths = {
        "Assets/Cubemaps/sky_34_cubemap_2k/px.png",
        "Assets/Cubemaps/sky_34_cubemap_2k/nx.png",
        "Assets/Cubemaps/sky_34_cubemap_2k/py.png",
        "Assets/Cubemaps/sky_34_cubemap_2k/ny.png",
        "Assets/Cubemaps/sky_34_cubemap_2k/pz.png",
        "Assets/Cubemaps/sky_34_cubemap_2k/nz.png",
    };

    engine_settings.UISystem = std::make_unique<Ignis::ImGuiSystem>();

    Ignis::Logger logger{};
    Ignis::Engine engine{};

    logger.initialize(logger_settings);
    engine.initialize(engine_settings);

    engine.pushLayer<Ignis::Editor>(Ignis::Editor::Settings{});

    engine.run();

    engine.shutdown();
    logger.shutdown();

    return 0;
}