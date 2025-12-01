#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Vulkan.hpp>

#include <Ignis/Render/FrameGraph.hpp>

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

        static FrameData &GetCurrentFrameData();

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

       private:
        static Render *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State);

       private:
        friend class Engine;
    };
}  // namespace Ignis