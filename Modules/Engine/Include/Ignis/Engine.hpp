#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/ECS.hpp>
#include <Ignis/Assets.hpp>
#include <Ignis/Window.hpp>
#include <Ignis/Vulkan.hpp>

namespace Ignis {
    class Engine {
       public:
        struct Settings {
            Window::Settings WindowSettings{};
        };

       public:
        static void Initialize(Engine *engine, const Settings &settings);
        static void Shutdown();

        static void Stop();
        static void Start();

        static bool IsRunning();

        static Entity CreateEntity();

       public:
        Engine()  = default;
        ~Engine() = default;

       private:
        IGNIS_IF_DEBUG(class State {
           public:
            ~State();
        };);

       private:
        SingletonManager m_SingletonManager;
        Window           m_Window;

        std::atomic<bool> m_IsRunning;

       private:
        static Engine *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State;)
    };
}  // namespace Ignis