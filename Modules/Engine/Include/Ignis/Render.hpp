#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Vulkan.hpp>

#include <Ignis/Render/FrameGraph.hpp>
#include <Ignis/Render/RenderPipeline.hpp>

namespace Ignis {
    class Render {
       public:
        struct Settings {
            uint32_t FramesInFlight = 3;
        };

        struct FrameData {
            vk::CommandPool   CommandPool;
            vk::CommandBuffer CommandBuffer;

            vk::Semaphore SwapchainSempahore;
            vk::Fence     RenderFence;

            uint32_t SwapchainImageIndex;
        };

       public:
        static void Initialize(Render *render, const Settings &settings);
        static void Shutdown();

        static void ImmediateSubmit(fu2::function<void(vk::CommandBuffer)> fn);

        static FrameData &GetCurrentFrameDataRef();

        static const FrameData &GetCurrentFrameDataConstRef();

        template <typename TRenderPipeline>
            requires(std::is_base_of_v<ARenderPipeline, TRenderPipeline>)
        static void removeRenderPipeline() {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Render is not initialized");
            const std::type_index type = typeid(TRenderPipeline);

            const auto it = s_pInstance->m_RenderPipelineLookUp.find(type);
            if (it == std::end(s_pInstance->m_RenderPipelineLookUp)) {
                DIGNIS_LOG_ENGINE_WARN("Ignis::Render::getRenderPipeline<{}>: RenderPipeline not added.", type.name());
                return;
            }

            const size_t index = it->second;

            s_pInstance->m_RenderPipelines[index].release();
            s_pInstance->m_RenderPipelines[index] = std::move(s_pInstance->m_RenderPipelines.back());

            s_pInstance->m_RenderPipelineLookUp[s_pInstance->m_RenderPipelines[index]->m_ID] = index;

            s_pInstance->m_RenderPipelines.pop_back();
            s_pInstance->m_RenderPipelineLookUp.erase(type);
        }

        template <typename TRenderPipeline, typename... Args>
            requires(std::is_base_of_v<ARenderPipeline, TRenderPipeline>)
        static TRenderPipeline *addRenderPipeline(Args &&...args) {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Render is not initialized");
            const std::type_index type = typeid(TRenderPipeline);
            DIGNIS_ASSERT(
                !s_pInstance->m_RenderPipelineLookUp.contains(type),
                "Ignis::Render::addRenderPipeline<{}>: render pipeline is already added.",
                type.name());

            s_pInstance->m_RenderPipelineLookUp[type] = s_pInstance->m_RenderPipelines.size();
            s_pInstance->m_RenderPipelines.emplace_back(std::make_unique<TRenderPipeline>(std::forward<Args>(args)...));

            return static_cast<TRenderPipeline *>(s_pInstance->m_RenderPipelines.back().get());
        }

        template <typename TRenderPipeline>
            requires(std::is_base_of_v<ARenderPipeline, TRenderPipeline>)
        static TRenderPipeline *getRenderPipeline() {
            DIGNIS_ASSERT(s_pInstance != nullptr, "Ignis::Render is not initialized");
            const std::type_index type = typeid(TRenderPipeline);

            const auto it = s_pInstance->m_RenderPipelineLookUp.find(type);
            if (it == std::end(s_pInstance->m_RenderPipelineLookUp)) {
                DIGNIS_LOG_ENGINE_WARN("Ignis::Render::getRenderPipeline<{}>: RenderPipeline not added.", type.name());
                return nullptr;
            }

            return static_cast<TRenderPipeline *>(s_pInstance->m_RenderPipelines[it->second].get());
        }

       public:
        Render()  = default;
        ~Render() = default;

       private:
        struct FrameImage {
            vk::Image     Handle;
            vk::ImageView View;
            vk::Extent2D  Extent;
        };

        IGNIS_IF_DEBUG(class State {
           public:
            ~State();
        });

       private:
        static void Resize(uint32_t width, uint32_t height);

        static std::optional<FrameImage> BeginFrame();

        static bool EndFrame(FrameGraph::Executor &&frame_graph_executor);

       private:
        uint32_t m_FramesInFlight;
        uint32_t m_FrameIndex;

        gtl::vector<FrameData> m_Frames;

        gtl::vector<vk::Semaphore> m_PresentSemaphores;

        vk::CommandPool   m_ImmCommandPool;
        vk::CommandBuffer m_ImmCommandBuffer;
        vk::Fence         m_ImmFence;

        gtl::vector<std::unique_ptr<ARenderPipeline>> m_RenderPipelines;
        gtl::flat_hash_map<std::type_index, size_t>   m_RenderPipelineLookUp;

       private:
        static Render *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State);

       private:
        friend class Engine;
    };
}  // namespace Ignis