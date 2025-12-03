#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Assets.hpp>
#include <Ignis/Camera.hpp>
#include <Ignis/Window.hpp>
#include <Ignis/Vulkan.hpp>
#include <Ignis/Render.hpp>

#include <Ignis/Engine/IGUISystem.hpp>
#include <Ignis/Engine/Layer.hpp>

#include <Ignis/GPUScenes/BlinnPhong.hpp>

namespace Ignis {
    class Engine {
       public:
        struct Settings {
            Window::Settings WindowSettings{};
            Vulkan::Settings VulkanSettings{};
            Render::Settings RenderSettings{};

            std::unique_ptr<IGUISystem> UISystem = nullptr;
        };

       public:
        static Engine &GetRef();

       public:
        Engine()  = default;
        ~Engine() = default;

        void initialize(Settings &settings);
        void shutdown();

        void run();
        void stop();

        bool isRunning() const;

        IGUISystem *getUISystem() const;

        FrameGraph &getFrameGraph();

        template <typename TUISystem>
            requires(std::is_base_of_v<IGUISystem, TUISystem>)
        TUISystem *getUISystem() const {
            DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized.");
            IGUISystem *ui_system = m_UISystem.get();
            return dynamic_cast<TUISystem *>(ui_system);
        }

        template <typename TLayer, typename... Args>
            requires(std::is_base_of_v<ALayer, TLayer>)
        TLayer *pushLayer(Args &&...args) {
            DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized.");
            const std::type_index layer_type = typeid(TLayer);
            DIGNIS_ASSERT(
                !m_LayerLookUp.contains(layer_type),
                "Ignis::Engine::pushLayer<{}>: Layer already exists.",
                layer_type.name());

            m_LayerLookUp[layer_type] = m_LayerStack.size();
            m_LayerStack.emplace_back(std::make_unique<TLayer>(std::forward<Args>(args)...));

            return static_cast<TLayer *>(m_LayerStack.back().get());
        }

        template <typename TLayer>
            requires(std::is_base_of_v<ALayer, TLayer>)
        TLayer *getLayer() {
            DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized.");
            const std::type_index layer_type = typeid(TLayer);

            const auto it = m_LayerLookUp.find(layer_type);
            if (it == std::end(m_LayerLookUp)) {
                DIGNIS_LOG_ENGINE_WARN("Ignis::Engine::getLayer<{}>: Layer not pushed.", layer_type.name());
                return nullptr;
            }

            return static_cast<TLayer *>(m_LayerStack[it->second].get());
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

        Window m_Window;
        Vulkan m_Vulkan;
        Render m_Render;

        std::unique_ptr<IGUISystem> m_UISystem;

        std::vector<std::unique_ptr<ALayer>>        m_LayerStack;
        gtl::flat_hash_map<std::type_index, size_t> m_LayerLookUp;

       private:
        friend class ALayer;

       private:
        static Engine *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State;)
    };
}  // namespace Ignis

#include <Ignis/Engine/LayerImpl.hpp>