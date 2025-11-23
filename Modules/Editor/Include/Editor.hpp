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
        void renderImGui();

        void drawFrame();
        void drawImGui(vk::ImageView target, vk::CommandBuffer cmd);

        void resize(uint32_t width, uint32_t height);

        void createRenderSemaphores();
        void releaseRenderSemaphores();

        void immediateSubmit(fu2::function<void(vk::CommandBuffer cmd)> &&function) const;

        void initializeImmData();
        void releaseImmData();

        void initializeImGuiData();
        void releaseImGuiData();

        void initializeDrawData();
        void releaseDrawData();

        void initializeDrawImage();
        void releaseDrawImage();

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

        std::vector<FrameData>     m_Frames;
        std::vector<vk::Semaphore> m_PresentSemaphores;

        uint32_t m_FrameIndex;
        uint32_t m_FramesInFlight;

        bool m_StopRendering;
        bool m_ResizeRequested;

        vk::Fence         m_ImmFence;
        vk::CommandPool   m_ImmCommandPool;
        vk::CommandBuffer m_ImmCommandBuffer;

        vk::DescriptorPool m_ImGuiDescriptorPool;

        Vulkan::Image::Allocation m_DrawImage;
        vk::ImageView             m_DrawImageView;

        vk::DescriptorPool m_DescriptorPool;
        vk::ShaderModule   m_ComputeShaderModule;

        vk::DescriptorSetLayout m_DrawImageDescriptorLayout;
        vk::DescriptorSet       m_DrawImageDescriptor;

        vk::PipelineLayout m_GradientPipelineLayout;
        vk::Pipeline       m_GradientPipeline;
    };
}  // namespace Ignis