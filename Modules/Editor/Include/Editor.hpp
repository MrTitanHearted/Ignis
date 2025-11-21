#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    class Editor {
       public:
        Editor()  = default;
        ~Editor() = default;

        void initialize(int32_t argc, const char **argv);
        void release();

        void run();

       private:
        struct FrameData {
            vk::CommandPool   CommandPool;
            vk::CommandBuffer CommandBuffer;

            vk::Semaphore SwapchainSemaphore;
            vk::Semaphore RenderSemaphore;

            vk::Fence RenderFence;
        };

       private:
        void drawFrame();

        void resize(uint32_t width, uint32_t height);

        FrameData &getCurrentFrameData();

       private:
        Logger m_Logger;
        Engine m_Engine;

        Timer  m_Timer;
        double m_DeltaTime;

        WindowEvents m_WindowEvents;

        Vulkan::Context m_VulkanContext;

        vk::Device       m_Device;
        vk::SurfaceKHR   m_Surface;
        uint32_t         m_QueueFamilyIndex;
        vk::Queue        m_GraphicsQueue;
        vk::Queue        m_ComputeQueue;
        vk::Queue        m_PresentQueue;
        vk::SwapchainKHR m_Swapchain;
        vk::Format       m_SwapchainFormat;
        vk::Extent2D     m_SwapchainExtent;
        vma::Allocator   m_VmaAllocator;

        std::vector<FrameData> m_Frames;

        uint32_t m_FrameIndex;
        uint32_t m_FramesInFlight;

        bool m_StopRendering;
    };
}  // namespace Ignis