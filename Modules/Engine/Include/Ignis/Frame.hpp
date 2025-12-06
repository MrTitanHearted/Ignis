#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Window.hpp>
#include <Ignis/Vulkan.hpp>

#include <Ignis/Frame/Graph.hpp>

namespace Ignis {
    class Frame {
       public:
        struct Settings {
            uint32_t FramesInFlight = 3;
        };

       public:
        static Frame &GetRef();

       public:
        Frame()  = default;
        ~Frame() = default;

        void initialize(const Settings &settings);
        void shutdown();

        FrameGraph &getFrameGraph();

       private:
        IGNIS_IF_DEBUG(class State {
           public:
            ~State();
        });

       private:
        struct Data {
            vk::CommandPool   CommandPool;
            vk::CommandBuffer CommandBuffer;

            vk::Semaphore SwapchainSemaphore;
            vk::Fence     RenderFence;

            uint32_t SwapchainImageIndex;
        };

       private:
        bool begin();
        bool end(FrameGraph::Executor &&frame_graph_executor);

        Data &getCurrentFrameDataRef();

       private:
        uint32_t m_FramesInFlight;
        uint32_t m_FrameIndex;

        std::vector<Data> m_Frames;

        std::vector<vk::Semaphore> m_PresentSemaphores;

        FrameGraph m_FrameGraph;

       private:
        static Frame *s_pInstance;

        IGNIS_IF_DEBUG(static State s_State);

       private:
        friend class Engine;
    };
}  // namespace Ignis