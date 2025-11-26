#pragma once

#include <Ignis/Engine/Layer.hpp>

#include <Ignis/Engine/WindowLayer.hpp>
#include <Ignis/Engine/VulkanLayer.hpp>

#include <Ignis/Engine/RenderLayer/FrameGraph.hpp>

namespace Ignis {
    class RenderLayer final : public ILayer {
       public:
        struct Settings {
            uint32_t FramesInFlight = 3u;
        };

        struct FrameData {
            vk::CommandPool   CommandPool;
            vk::CommandBuffer CommandBuffer;

            vk::Semaphore SwapchainSemaphore;
            vk::Fence     RenderFence;

            uint32_t SwapchainImageIndex;

            vk::Image     Image;
            vk::ImageView ImageView;
            vk::Extent2D  ImageExtent;

            FrameGraph::Builder FrameGraphBuilder;
            FrameGraph::ImageID SwapchainImageID;
        };

        struct RenderTarget {
            vk::ImageView View;
            vk::Extent2D  Extent;
        };

       public:
        explicit RenderLayer(VulkanLayer *vulkan_layer, const Settings &settings);
        ~RenderLayer() override;

        void onEvent(AEvent &event) override;

        void onPreRender() override;
        void onRender() override;
        void onPostRender() override;

        void immediateSubmit(fu2::function<void(vk::CommandBuffer cmd)> fn) const;

        bool onWindowResizeEvent(const WindowResizeEvent &resize_event);

        bool shouldSkipFrame() const;

        FrameData &getCurrentFrameData();

       private:
        void createPresentSemaphores();
        void destroyPresentSemaphores();

       private:
        VulkanLayer *m_pVulkanLayer;

        vk::SurfaceKHR m_Surface;

        vk::Device m_Device;
        uint32_t   m_QueueFamilyIndex;
        vk::Queue  m_GraphicsQueue;
        vk::Queue  m_ComputeQueue;
        vk::Queue  m_PresentQueue;

        uint32_t     m_SwapchainImageCount;
        vk::Extent2D m_SwapchainExtent;
        vk::Format   m_SwapchainFormat;

        vk::SwapchainKHR m_Swapchain;

        std::vector<vk::Semaphore> m_PresentSemaphores;

        vk::CommandPool   m_ImmCommandPool;
        vk::CommandBuffer m_ImmCommandBuffer;
        vk::Fence         m_ImmFence;

        uint32_t               m_FramesInFlight;
        std::vector<FrameData> m_Frames;

        uint32_t m_FrameIndex;
    };
}  // namespace Ignis