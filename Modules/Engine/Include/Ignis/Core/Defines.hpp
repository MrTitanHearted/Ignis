#pragma once

#define IGNIS_RELATIVE_FILE_ (&__FILE__[std::strlen(IGNIS_PROJECT_ROOT_DIR) + 1])

#if defined(_WIN32) || defined(_WIN64)
    #define IGNIS_PLATFORM_WINDOWS
#elif defined(__linux__)
    #define IGNIS_PLATFORM_UNIX
    #define IGNIS_PLATFORM_LINUX
#else
    #error Unsupported platform!
#endif

#if !defined(IGNIS_BUILD_TYPE_RELEASE) && !defined(IGNIS_BUILD_TYPE_DEBUG)
    #define IGNIS_BUILD_TYPE_DEBUG
#endif

#ifdef IGNIS_BUILD_TYPE_DEBUG
    #ifndef IGNIS_IF_DEBUG
        #define IGNIS_IF_DEBUG(stmt) stmt
    #endif
#else
    #ifndef IGNIS_IF_DEBUG
        #define IGNIS_IF_DEBUG(stmt)
    #endif
#endif