#include <Ignis/Core/Logger.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>

namespace Ignis {
    Logger *Logger::s_pLogger = nullptr;

    IGNIS_IF_DEBUG(Logger::State Logger::s_State{});

    void Logger::Initialize(Logger *logger, const Settings &settings) {
        assert(s_pLogger == nullptr && "Ignis::Logger already initialized");

        s_pLogger = logger;

        s_pLogger->m_FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(settings.EngineLogFileName, true);
        s_pLogger->m_FileSink->set_pattern(GetFileSinkPattern());

        s_pLogger->m_ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        s_pLogger->m_ConsoleSink->set_pattern(GetConsoleSinkPattern());

        s_pLogger->m_EngineLogger      = std::make_shared<spdlog::logger>("ENGINE", spdlog::sinks_init_list{s_pLogger->m_FileSink, s_pLogger->m_ConsoleSink});
        s_pLogger->m_ApplicationLogger = std::make_shared<spdlog::logger>(settings.ApplicationLogName, spdlog::sinks_init_list{s_pLogger->m_ConsoleSink});

        s_pLogger->m_EngineLogger->set_level(settings.EngineLogLevel);
        s_pLogger->m_ApplicationLogger->set_level(settings.ApplicationLogLevel);

        for (const auto &sink : settings.EngineSinks) {
            s_pLogger->m_EngineLogger->sinks().push_back(sink);
        }
        for (const auto &sink : settings.ApplicationSinks) {
            s_pLogger->m_ApplicationLogger->sinks().push_back(sink);
        }

        spdlog::register_logger(s_pLogger->m_EngineLogger);
        spdlog::register_logger(s_pLogger->m_ApplicationLogger);

        DIGNIS_LOG_ENGINE_INFO("Ignis::Logger Initialized");
    }

    void Logger::Shutdown() {
        assert(s_pLogger != nullptr && "Ignis::Logger is not initialized");

        DIGNIS_LOG_ENGINE_INFO("Ignis::Logger Shutdown");

        spdlog::drop_all();
        spdlog::shutdown();

        s_pLogger->m_EngineLogger      = nullptr;
        s_pLogger->m_ApplicationLogger = nullptr;
        s_pLogger->m_FileSink          = nullptr;
        s_pLogger->m_ConsoleSink       = nullptr;

        s_pLogger = nullptr;
    }

    std::shared_ptr<spdlog::logger> Logger::GetEngineLogger() {
        IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));
        return s_pLogger->m_EngineLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::GetApplicationLogger() {
        IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));
        return s_pLogger->m_ApplicationLogger;
    }

    std::shared_ptr<spdlog::sinks::sink> Logger::GetFileSink() {
        IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));
        return s_pLogger->m_FileSink;
    }

    std::shared_ptr<spdlog::sinks::sink> Logger::GetConsoleSink() {
        IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));
        return s_pLogger->m_ConsoleSink;
    }

    const std::string &Logger::GetFileSinkPattern() {
        static std::string pattern = "[%Y-%m-%d %T]::[%@]::[%!]::[%l]: %v";
        return pattern;
    }

    const std::string &Logger::GetConsoleSinkPattern() {
        static std::string pattern = "[%n] [%^%l%$] [%s:%#] %v";
        return pattern;
    }

    IGNIS_IF_DEBUG(Logger::State::~State() {
        assert(s_pLogger == nullptr && "Forgot to shutdown Ignis::Logger");
    })
}  // namespace Ignis