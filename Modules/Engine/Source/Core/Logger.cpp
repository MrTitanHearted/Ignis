#include <Ignis/Core/Logger.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>

namespace Ignis {
    Logger *Logger::s_pLogger = nullptr;

    IGNIS_IF_DEBUG(Logger::State Logger::s_State{});

    Logger &Logger::GetRef() {
        assert(s_pLogger != nullptr && "Ignis::Logger is not initialized");
        return *s_pLogger;
    }

    const std::string &Logger::GetFileSinkPattern() {
        static std::string pattern = "[%Y-%m-%d %T]::[%@]::[%!]::[%l]: %v";
        return pattern;
    }

    const std::string &Logger::GetConsoleSinkPattern() {
        static std::string pattern = "[%n] [%^%l%$] [%s:%#] %v";
        return pattern;
    }

    void Logger::initialize(const Settings &settings) {
        assert(s_pLogger == nullptr && "Ignis::Logger already initialized");

        m_FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(settings.EngineLogFileName, true);
        m_FileSink->set_pattern(GetFileSinkPattern());

        m_ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        m_ConsoleSink->set_pattern(GetConsoleSinkPattern());

        m_EngineLogger      = std::make_shared<spdlog::logger>("ENGINE", spdlog::sinks_init_list{m_FileSink, m_ConsoleSink});
        m_ApplicationLogger = std::make_shared<spdlog::logger>(settings.ApplicationLogName, spdlog::sinks_init_list{m_ConsoleSink});

        m_EngineLogger->set_level(settings.EngineLogLevel);
        m_ApplicationLogger->set_level(settings.ApplicationLogLevel);

        for (const auto &sink : settings.EngineSinks) {
            m_EngineLogger->sinks().push_back(sink);
        }
        for (const auto &sink : settings.ApplicationSinks) {
            m_ApplicationLogger->sinks().push_back(sink);
        }

        spdlog::register_logger(m_EngineLogger);
        spdlog::register_logger(m_ApplicationLogger);

        s_pLogger = this;

        DIGNIS_LOG_ENGINE_INFO("Ignis::Logger Initialized");
    }

    void Logger::shutdown() {
        assert(s_pLogger != nullptr && "Ignis::Logger is not initialized");

        DIGNIS_LOG_ENGINE_INFO("Ignis::Logger Shutdown");

        spdlog::drop_all();
        spdlog::shutdown();

        m_EngineLogger      = nullptr;
        m_ApplicationLogger = nullptr;
        m_FileSink          = nullptr;
        m_ConsoleSink       = nullptr;

        s_pLogger = nullptr;
    }

    std::shared_ptr<spdlog::logger> Logger::getEngineLogger() const {
        IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));
        return m_EngineLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getApplicationLogger() const {
        IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));
        return m_ApplicationLogger;
    }

    std::shared_ptr<spdlog::sinks::sink> Logger::getFileSink() const {
        IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));
        return m_FileSink;
    }

    std::shared_ptr<spdlog::sinks::sink> Logger::getConsoleSink() const {
        IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));
        return m_ConsoleSink;
    }

    IGNIS_IF_DEBUG(Logger::State::~State() {
        assert(s_pLogger == nullptr && "Forgot to shutdown Ignis::Logger");
    })
}  // namespace Ignis