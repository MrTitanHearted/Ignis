#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Assets.hpp>
#include <Ignis/Camera.hpp>
#include <Ignis/Window.hpp>
#include <Ignis/Vulkan.hpp>
#include <Ignis/Frame.hpp>

#include <Ignis/Engine/GUISystem.hpp>
#include <Ignis/Engine/Layer.hpp>
#include <Ignis/Engine/RenderModule.hpp>

namespace Ignis {
    class Engine {
       public:
        struct Settings {
            Window::Settings WindowSettings{};
            Vulkan::Settings VulkanSettings{};
            Frame::Settings  FrameSettings{};

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

        FrameGraph &getFrameGraph();

        template <typename TUISystem>
            requires(std::is_base_of_v<IGUISystem, TUISystem>)
        TUISystem *getUISystem() const {
            DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized.");
            IGUISystem *ui_system = m_GUISystem.get();
            return dynamic_cast<TUISystem *>(ui_system);
        }

        template <typename TLayer, typename... Args>
            requires(std::is_base_of_v<ILayer, TLayer>)
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
            requires(std::is_base_of_v<ILayer, TLayer>)
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

        template <typename TRenderModule>
            requires(std::is_base_of_v<IRenderModule, TRenderModule>)
        void removeRenderModule() {
            DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized.");
            const std::type_index type = typeid(TRenderModule);

            DIGNIS_ASSERT(
                m_RenderModules.find(type) != std::end(m_RenderModules),
                "Ignis::Engine::removeRenderModule<{}>: RenderModule not found.",
                std::string_view{type.name()});

            m_RenderModules[type]->onDetach(m_Frame.getFrameGraph());
            m_RenderModules.erase(type);
        }

        template <typename TRenderModule, typename... Args>
            requires(std::is_base_of_v<IRenderModule, TRenderModule>)
        TRenderModule *addRenderModule(Args &&...args) {
            DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized.");
            const std::type_index type = typeid(TRenderModule);

            DIGNIS_ASSERT(
                m_RenderModules.find(type) == std::end(m_RenderModules),
                "Ignis::Engine::addRenderModule<{}>: RenderModule already added.",
                std::string_view{type.name()});

            m_RenderModules[type] = std::make_unique<TRenderModule>(std::forward<Args>(args)...);
            m_RenderModules[type]->onAttach(m_Frame.getFrameGraph());

            return static_cast<TRenderModule *>(m_RenderModules[type].get());
        }

        template <typename TRenderModule, typename... Args>
            requires(std::is_base_of_v<IRenderModule, TRenderModule>)
        TRenderModule *getRenderModule() {
            DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Engine is not initialized.");
            const std::type_index type = typeid(TRenderModule);

            DIGNIS_ASSERT(
                m_RenderModules.find(type) != std::end(m_RenderModules),
                "Ignis::Engine::removeRenderModule<{}>: RenderModule not found.",
                std::string_view{type.name()});

            return static_cast<TRenderModule *>(m_RenderModules[type].get());
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
        Frame  m_Frame;

        std::unique_ptr<IGUISystem> m_GUISystem;

        std::vector<std::unique_ptr<ILayer>>        m_LayerStack;
        gtl::flat_hash_map<std::type_index, size_t> m_LayerLookUp;

        gtl::flat_hash_map<std::type_index, std::unique_ptr<IRenderModule>> m_RenderModules;

       private:
        friend class IGUISystem;
        friend class ILayer;

       private:
        static Engine *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State;)
    };
}  // namespace Ignis

#include <Ignis/Engine/GUISystemImpl.hpp>
#include <Ignis/Engine/LayerImpl.hpp>
