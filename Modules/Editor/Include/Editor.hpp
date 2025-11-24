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

            vk::Fence RenderFence;
        };

       private:
        void createRenderSemaphores();
        void destroyRenderSemaphores();

        void initializeImmData();
        void releaseImmData();

        void initializeImGui();
        void releaseImGui();

        void createViewportImage(uint32_t width, uint32_t height);
        void destroyViewportImage();

        void renderImGui();

        void drawFrame();
        void drawGeometry(
            vk::ImageView       target,
            const vk::Extent2D &extent,
            vk::CommandBuffer   cmd) const;
        void drawImGui(
            vk::ImageView       target,
            const vk::Extent2D &extent,
            vk::CommandBuffer   cmd) const;

        void resize(uint32_t width, uint32_t height);

        void immediateSubmit(fu2::function<void(vk::CommandBuffer cmd)> &&function) const;

        FrameData &getCurrentFrameData();

       private:
        struct Vertex {
            glm::vec4 Position;
            glm::vec4 Color;
        };

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

        uint32_t m_FrameIndex;
        uint32_t m_FramesInFlight;

        std::vector<FrameData>     m_Frames;
        std::vector<vk::Semaphore> m_PresentSemaphores;

        vk::Fence         m_ImmFence;
        vk::CommandPool   m_ImmCommandPool;
        vk::CommandBuffer m_ImmCommandBuffer;

        vk::DescriptorPool m_ImGuiDescriptorPool;

        vk::DescriptorSet m_ViewportImageDescriptor;

        Vulkan::Image::Allocation m_ViewportImage;

        vk::Sampler   m_ViewportImageSampler;
        vk::ImageView m_ViewportImageView;
        vk::Format    m_ViewportFormat;
        vk::Extent2D  m_ViewportExtent;

        bool m_ResizeRequested;
    };
}  // namespace Ignis