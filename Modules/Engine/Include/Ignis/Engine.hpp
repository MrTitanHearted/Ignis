#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/ECS.hpp>
#include <Ignis/Assets.hpp>

#include <Ignis/Engine/Event.hpp>
#include <Ignis/Engine/Layer.hpp>

#include <Ignis/Engine/WindowLayer.hpp>
#include <Ignis/Engine/VulkanLayer.hpp>
#include <Ignis/Engine/RenderLayer.hpp>

namespace Ignis {
    class Engine {
       public:
        struct Settings {
            WindowLayer::Settings WindowSettings{};
            VulkanLayer::Settings VulkanSettings{};
            RenderLayer::Settings RenderSettings{};
        };

       public:
        static void Initialize(Engine *engine, const Settings &settings);
        static void Shutdown();

        static Engine &Get();

       public:
        Engine()  = default;
        ~Engine() = default;

        void run();
        void stop();

        static void raiseEvent(AEvent &event);

        static bool isRunning();

        template <typename TLayer, typename... Args, typename = std::enable_if_t<std::is_base_of_v<ILayer, TLayer>>>
        void pushLayer(Args &&...args) {
            const std::type_index type = typeid(TLayer);

            m_LayerLookUp[type] = m_LayerStack.size();
            m_LayerTypes.push_back(type);
            m_LayerStack.push_back(std::make_unique<TLayer>(std::forward<Args>(args)...));
        }

        template <typename TLayer, typename = std::enable_if_t<std::is_base_of_v<ILayer, TLayer>>>
        TLayer *getLayer() {
            const std::type_index type = typeid(TLayer);

            const auto iter = m_LayerLookUp.find(type);
            if (iter == m_LayerLookUp.end())
                return nullptr;

            return static_cast<TLayer *>(m_LayerStack[iter->second].get());
        }

       private:
        IGNIS_IF_DEBUG(class State {
           public:
            ~State();
        };);

       private:
        std::atomic<bool> m_IsRunning;

        Timer  m_Timer;
        double m_DeltaTime;

        std::vector<std::unique_ptr<ILayer>>        m_LayerStack;
        std::vector<std::type_index>                m_LayerTypes;
        gtl::flat_hash_map<std::type_index, size_t> m_LayerLookUp;

       private:
        friend class ILayer;

       private:
        static Engine *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State;)
    };
}  // namespace Ignis