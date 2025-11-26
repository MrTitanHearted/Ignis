#pragma once

#include <Ignis/Core/PCH.hpp>

#ifndef IGNIS_LOGGER_CALL
    #define IGNIS_LOGGER_CALL(logger, level, fmt, ...) (logger)->log(spdlog::source_loc{IGNIS_RELATIVE_FILE_, __LINE__, SPDLOG_FUNCTION}, level, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOGGER_CRITICAL
    #define IGNIS_LOGGER_CRITICAL(logger, fmt, ...) IGNIS_LOGGER_CALL(logger, spdlog::level::critical, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOGGER_ERROR
    #define IGNIS_LOGGER_ERROR(logger, fmt, ...) IGNIS_LOGGER_CALL(logger, spdlog::level::err, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOGGER_DEBUG
    #define IGNIS_LOGGER_DEBUG(logger, fmt, ...) IGNIS_LOGGER_CALL(logger, spdlog::level::debug, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOGGER_WARN
    #define IGNIS_LOGGER_WARN(logger, fmt, ...) IGNIS_LOGGER_CALL(logger, spdlog::level::warn, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOGGER_INFO
    #define IGNIS_LOGGER_INFO(logger, fmt, ...) IGNIS_LOGGER_CALL(logger, spdlog::level::info, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOGGER_TRACE
    #define IGNIS_LOGGER_TRACE(logger, fmt, ...) IGNIS_LOGGER_CALL(logger, spdlog::level::trace, fmt, ##__VA_ARGS__)
#endif

#ifndef IGNIS_LOG_ENGINE_CALL
    #define IGNIS_LOG_ENGINE_CALL(level, fmt, ...) IGNIS_LOGGER_CALL(::Ignis::Logger::GetEngineLogger(), level, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_ENGINE_CRITICAL
    #define IGNIS_LOG_ENGINE_CRITICAL(fmt, ...) IGNIS_LOG_ENGINE_CALL(spdlog::level::critical, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_ENGINE_ERROR
    #define IGNIS_LOG_ENGINE_ERROR(fmt, ...) IGNIS_LOG_ENGINE_CALL(spdlog::level::err, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_ENGINE_DEBUG
    #define IGNIS_LOG_ENGINE_DEBUG(fmt, ...) IGNIS_LOG_ENGINE_CALL(spdlog::level::debug, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_ENGINE_WARN
    #define IGNIS_LOG_ENGINE_WARN(fmt, ...) IGNIS_LOG_ENGINE_CALL(spdlog::level::warn, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_ENGINE_INFO
    #define IGNIS_LOG_ENGINE_INFO(fmt, ...) IGNIS_LOG_ENGINE_CALL(spdlog::level::info, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_ENGINE_TRACE
    #define IGNIS_LOG_ENGINE_TRACE(fmt, ...) IGNIS_LOG_ENGINE_CALL(spdlog::level::trace, fmt, ##__VA_ARGS__)
#endif

#ifndef IGNIS_LOG_APPLICATION_CALL
    #define IGNIS_LOG_APPLICATION_CALL(level, fmt, ...) IGNIS_LOGGER_CALL(Ignis::Logger::GetApplicationLogger(), level, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_APPLICATION_CRITICAL
    #define IGNIS_LOG_APPLICATION_CRITICAL(fmt, ...) IGNIS_LOG_APPLICATION_CALL(spdlog::level::critical, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_APPLICATION_ERROR
    #define IGNIS_LOG_APPLICATION_ERROR(fmt, ...) IGNIS_LOG_APPLICATION_CALL(spdlog::level::err, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_APPLICATION_DEBUG
    #define IGNIS_LOG_APPLICATION_DEBUG(fmt, ...) IGNIS_LOG_APPLICATION_CALL(spdlog::level::debug, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_APPLICATION_WARN
    #define IGNIS_LOG_APPLICATION_WARN(fmt, ...) IGNIS_LOG_APPLICATION_CALL(spdlog::level::warn, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_APPLICATION_INFO
    #define IGNIS_LOG_APPLICATION_INFO(fmt, ...) IGNIS_LOG_APPLICATION_CALL(spdlog::level::info, fmt, ##__VA_ARGS__)
#endif
#ifndef IGNIS_LOG_APPLICATION_TRACE
    #define IGNIS_LOG_APPLICATION_TRACE(fmt, ...) IGNIS_LOG_APPLICATION_CALL(spdlog::level::trace, fmt, ##__VA_ARGS__)
#endif

#ifdef IGNIS_BUILD_TYPE_DEBUG
    #ifndef DIGNIS_LOGGER_CALL
        #define DIGNIS_LOGGER_CALL(logger, level, fmt, ...) IGNIS_LOGGER_CALL(logger, level, fmt, ##__VA_ARGS__)
    #endif
#else
    #ifndef DIGNIS_LOGGER_CALL
        #define DIGNIS_LOGGER_CALL(logger, level, fmt, ...)
    #endif
#endif

#ifndef DIGNIS_LOGGER_CRITICAL
    #define DIGNIS_LOGGER_CRITICAL(logger, fmt, ...) DIGNIS_LOGGER_CALL(logger, spdlog::level::critical, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOGGER_ERROR
    #define DIGNIS_LOGGER_ERROR(logger, fmt, ...) DIGNIS_LOGGER_CALL(logger, spdlog::level::err, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOGGER_DEBUG
    #define DIGNIS_LOGGER_DEBUG(logger, fmt, ...) DIGNIS_LOGGER_CALL(logger, spdlog::level::debug, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOGGER_WARN
    #define DIGNIS_LOGGER_WARN(logger, fmt, ...) DIGNIS_LOGGER_CALL(logger, spdlog::level::warn, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOGGER_INFO
    #define DIGNIS_LOGGER_INFO(logger, fmt, ...) DIGNIS_LOGGER_CALL(logger, spdlog::level::info, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOGGER_TRACE
    #define DIGNIS_LOGGER_TRACE(logger, fmt, ...) DIGNIS_LOGGER_CALL(logger, spdlog::level::trace, fmt, ##__VA_ARGS__)
#endif

#ifndef DIGNIS_LOG_ENGINE_CALL
    #define DIGNIS_LOG_ENGINE_CALL(level, fmt, ...) DIGNIS_LOGGER_CALL(::Ignis::Logger::GetEngineLogger(), level, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_ENGINE_CRITICAL
    #define DIGNIS_LOG_ENGINE_CRITICAL(fmt, ...) DIGNIS_LOG_ENGINE_CALL(spdlog::level::critical, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_ENGINE_ERROR
    #define DIGNIS_LOG_ENGINE_ERROR(fmt, ...) DIGNIS_LOG_ENGINE_CALL(spdlog::level::err, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_ENGINE_DEBUG
    #define DIGNIS_LOG_ENGINE_DEBUG(fmt, ...) DIGNIS_LOG_ENGINE_CALL(spdlog::level::debug, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_ENGINE_WARN
    #define DIGNIS_LOG_ENGINE_WARN(fmt, ...) DIGNIS_LOG_ENGINE_CALL(spdlog::level::warn, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_ENGINE_INFO
    #define DIGNIS_LOG_ENGINE_INFO(fmt, ...) DIGNIS_LOG_ENGINE_CALL(spdlog::level::info, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_ENGINE_TRACE
    #define DIGNIS_LOG_ENGINE_TRACE(fmt, ...) DIGNIS_LOG_ENGINE_CALL(spdlog::level::trace, fmt, ##__VA_ARGS__)
#endif

#ifndef DIGNIS_LOG_APPLICATION_CALL
    #define DIGNIS_LOG_APPLICATION_CALL(level, fmt, ...) DIGNIS_LOGGER_CALL(Ignis::Logger::GetApplicationLogger(), level, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_APPLICATION_CRITICAL
    #define DIGNIS_LOG_APPLICATION_CRITICAL(fmt, ...) DIGNIS_LOG_APPLICATION_CALL(spdlog::level::critical, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_APPLICATION_ERROR
    #define DIGNIS_LOG_APPLICATION_ERROR(fmt, ...) DIGNIS_LOG_APPLICATION_CALL(spdlog::level::err, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_APPLICATION_DEBUG
    #define DIGNIS_LOG_APPLICATION_DEBUG(fmt, ...) DIGNIS_LOG_APPLICATION_CALL(spdlog::level::debug, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_APPLICATION_WARN
    #define DIGNIS_LOG_APPLICATION_WARN(fmt, ...) DIGNIS_LOG_APPLICATION_CALL(spdlog::level::warn, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_APPLICATION_INFO
    #define DIGNIS_LOG_APPLICATION_INFO(fmt, ...) DIGNIS_LOG_APPLICATION_CALL(spdlog::level::info, fmt, ##__VA_ARGS__)
#endif
#ifndef DIGNIS_LOG_APPLICATION_TRACE
    #define DIGNIS_LOG_APPLICATION_TRACE(fmt, ...) DIGNIS_LOG_APPLICATION_CALL(spdlog::level::trace, fmt, ##__VA_ARGS__)
#endif

#ifndef IGNIS_ASSERT
    #define IGNIS_ASSERT(condition, ...) Ignis::Logger::Assert((condition), #condition, spdlog::source_loc{IGNIS_RELATIVE_FILE_, __LINE__, SPDLOG_FUNCTION}, ##__VA_ARGS__)
#endif

#ifndef DIGNIS_ASSERT
    #ifdef IGNIS_BUILD_TYPE_DEBUG
        #define DIGNIS_ASSERT(condition, ...) IGNIS_ASSERT(condition, ##__VA_ARGS__)
    #else
        #define DIGNIS_ASSERT(condition, ...)
    #endif
#endif

namespace Ignis {
    template <typename... FormatArgs>
    std::string formatStr(const std::string_view fmt, FormatArgs &&...args) {
        return std::vformat(fmt, std::make_format_args(args...));
    }

    class Logger {
       public:
        struct Settings {
            spdlog::level::level_enum EngineLogLevel      = spdlog::level::warn;
            spdlog::level::level_enum ApplicationLogLevel = spdlog::level::trace;

            std::string EngineLogFileName  = "IGNIS_ENGINE.log";
            std::string ApplicationLogName = "APPLICATION";

            std::vector<spdlog::sink_ptr> EngineSinks      = {};
            std::vector<spdlog::sink_ptr> ApplicationSinks = {};
        };

       public:
        static void Initialize(Logger *logger, const Settings &settings);
        static void Shutdown();

        static std::shared_ptr<spdlog::logger> GetEngineLogger();
        static std::shared_ptr<spdlog::logger> GetApplicationLogger();

        static std::shared_ptr<spdlog::sinks::sink> GetFileSink();
        static std::shared_ptr<spdlog::sinks::sink> GetConsoleSink();

        static const std::string &GetFileSinkPattern();
        static const std::string &GetConsoleSinkPattern();

       public:
        template <typename... Args>
        static void Assert(
            const bool         condition,
            const std::string &conditionStr,
            spdlog::source_loc location,
            Args &&...args) {
            IGNIS_IF_DEBUG(assert(s_pLogger != nullptr && "Ignis::Logger is not initialized"));

            if (condition) return;

            if constexpr (sizeof...(args) > 0) {
                s_pLogger->m_EngineLogger->log(
                    location, spdlog::level::critical,
                    "Assertion failed for: '{}' with message: '{}'",
                    conditionStr,
                    formatStr(std::forward<Args>(args)...));
            } else {
                s_pLogger->m_EngineLogger->log(
                    location, spdlog::level::critical,
                    "Assertion failed for: '{}'",
                    conditionStr);
            }

            std::abort();
        }

       public:
        Logger()  = default;
        ~Logger() = default;

       private:
        IGNIS_IF_DEBUG(class State {
           public:
            State() = default;
            ~State();
        });

       private:
        std::shared_ptr<spdlog::logger> m_EngineLogger;
        std::shared_ptr<spdlog::logger> m_ApplicationLogger;

        std::shared_ptr<spdlog::sinks::sink> m_FileSink;
        std::shared_ptr<spdlog::sinks::sink> m_ConsoleSink;

       private:
        static Logger *s_pLogger;

        IGNIS_IF_DEBUG(static State s_State);
    };
}  // namespace Ignis