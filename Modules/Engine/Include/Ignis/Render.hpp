#pragma once

#include <Ignis/Core.hpp>

#include <Ignis/Render/FrameGraph.hpp>
#include <Ignis/Render/GPUScene.hpp>

namespace Ignis {
    class Render {
       public:
        struct Settings {
            uint32_t FramesInFlight = 3;
        };

        struct FrameData {
            vk::CommandPool   CommandPool;
            vk::CommandBuffer CommandBuffer;

            vk::Semaphore SwapchainSemaphore;
            vk::Fence     RenderFence;

            uint32_t SwapchainImageIndex;
        };

       public:
        static Render &GetRef();

        static void ImmediateSubmit(const fu2::function<void(vk::CommandBuffer)> &fn);

       public:
        Render()  = default;
        ~Render() = default;

        void initialize(const Settings &settings);
        void shutdown();

        void immediateSubmit(fu2::function<void(vk::CommandBuffer)> fn) const;

        FrameGraph &getFrameGraph();

        FrameData &getCurrentFrameDataRef();

        template <typename TGPUScene>
            requires(std::is_base_of_v<AGPUScene, TGPUScene>)
        void removeGPUScene() {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Render is not initialized");
            const std::type_index type = typeid(TGPUScene);

            const auto it = m_GPUSceneLookUp.find(type);
            if (it == std::end(m_GPUSceneLookUp)) {
                DIGNIS_LOG_ENGINE_WARN("Ignis::Render::getGPUScene<{}>: GPUScene not added.", type.name());
                return;
            }

            const size_t index = it->second;

            m_GPUScenes[index]->release(m_FrameGraph);
            m_GPUScenes[index].release();
            m_GPUScenes[index] = std::move(m_GPUScenes.back());

            m_GPUSceneLookUp[m_GPUScenes[index]->m_ID] = index;

            m_GPUScenes.pop_back();
            m_GPUSceneLookUp.erase(type);
        }

        template <typename TGPUScene, typename... Args>
            requires(std::is_base_of_v<AGPUScene, TGPUScene>)
        TGPUScene *addGPUScene(Args &&...args) {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Render is not initialized");
            const std::type_index type = typeid(TGPUScene);
            DIGNIS_ASSERT(
                !m_GPUSceneLookUp.contains(type),
                "Ignis::Render::addGPUScene<{}>: render pipeline is already added.",
                type.name());

            m_GPUSceneLookUp[type] = m_GPUScenes.size();
            m_GPUScenes.emplace_back(std::make_unique<TGPUScene>(std::forward<Args>(args)...));
            m_GPUScenes.back()->initialize(m_FrameGraph);

            return static_cast<TGPUScene *>(m_GPUScenes.back().get());
        }

        template <typename TGPUScene>
            requires(std::is_base_of_v<AGPUScene, TGPUScene>)
        TGPUScene *getGPUScene() {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Render is not initialized");
            const std::type_index type = typeid(TGPUScene);

            const auto it = m_GPUSceneLookUp.find(type);
            if (it == std::end(m_GPUSceneLookUp)) {
                DIGNIS_LOG_ENGINE_WARN("Ignis::Render::getGPUScene<{}>: GPUScene not added.", type.name());
                return nullptr;
            }

            return static_cast<TGPUScene *>(m_GPUScenes[it->second].get());
        }

       private:
        IGNIS_IF_DEBUG(class State {
           public:
            ~State();
        });

       private:
        void resize(uint32_t width, uint32_t height);

        bool beginFrame();
        bool endFrame(FrameGraph::Executor &&frame_graph_executor);

       private:
        uint32_t m_FramesInFlight;
        uint32_t m_FrameIndex;

        std::vector<FrameData> m_Frames;

        std::vector<vk::Semaphore> m_PresentSemaphores;

        vk::CommandPool   m_ImmCommandPool;
        vk::CommandBuffer m_ImmCommandBuffer;
        vk::Fence         m_ImmFence;

        FrameGraph m_FrameGraph;

        std::vector<std::unique_ptr<AGPUScene>>     m_GPUScenes;
        gtl::flat_hash_map<std::type_index, size_t> m_GPUSceneLookUp;

       private:
        static Render *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State);

       private:
        friend class Engine;
    };
}  // namespace Ignis