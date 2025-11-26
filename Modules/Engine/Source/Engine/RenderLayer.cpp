// ReSharper disable CppDFANullDereference
#include <Ignis/Engine/RenderLayer.hpp>

namespace Ignis {
    RenderLayer::RenderLayer(VulkanLayer *vulkan_layer, const Settings &settings) {
        DIGNIS_ASSERT(nullptr != vulkan_layer);

        m_pVulkanLayer = vulkan_layer;

        m_Surface = m_pVulkanLayer->getSurface();

        m_Device           = m_pVulkanLayer->getDevice();
        m_QueueFamilyIndex = m_pVulkanLayer->getQueueFamilyIndex();
        m_GraphicsQueue    = m_pVulkanLayer->getGraphicsQueue();
        m_ComputeQueue     = m_pVulkanLayer->getComputeQueue();
        m_PresentQueue     = m_pVulkanLayer->getPresentQueue();

        m_SwapchainImageCount = m_pVulkanLayer->getSwapchainImageCount();
        m_SwapchainExtent     = m_pVulkanLayer->getSwapchainExtent();
        m_SwapchainFormat     = m_pVulkanLayer->getSwapchainFormat().format;

        m_Swapchain = m_pVulkanLayer->getSwapchain();

        createPresentSemaphores();

        m_ImmCommandPool   = Vulkan::CommandPool::CreateResetCommandBuffer(m_QueueFamilyIndex, m_Device);
        m_ImmCommandBuffer = Vulkan::CommandBuffer::AllocatePrimary(m_ImmCommandPool, m_Device);
        m_ImmFence         = Vulkan::Fence::CreateSignaled(m_Device);

        m_FramesInFlight = settings.FramesInFlight;
        m_FrameIndex     = 0;

        m_Frames.clear();
        m_Frames.reserve(m_FramesInFlight);

        for (uint32_t i = 0; i < m_FramesInFlight; i++) {
            FrameData frame_data{};
            frame_data.CommandPool   = Vulkan::CommandPool::CreateResetCommandBuffer(m_QueueFamilyIndex, m_Device);
            frame_data.CommandBuffer = Vulkan::CommandBuffer::AllocatePrimary(m_ImmCommandPool, m_Device);
            frame_data.RenderFence   = Vulkan::Fence::CreateSignaled(m_Device);

            frame_data.SwapchainSemaphore = Vulkan::Semaphore::Create(m_Device);

            frame_data.Image       = nullptr;
            frame_data.ImageView   = nullptr;
            frame_data.ImageExtent = m_SwapchainExtent;

            m_Frames.push_back(frame_data);
        }

        immediateSubmit([this](const vk::CommandBuffer cmd) {
            for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
                Vulkan::Image::TransitionLayout(m_pVulkanLayer->getSwapchainImage(i), vk::ImageLayout::ePresentSrcKHR, cmd);
            }
        });

        DIGNIS_LOG_ENGINE_INFO("Ignis::RenderLayer Initialized");
    }

    RenderLayer::~RenderLayer() {
        DIGNIS_VK_CHECK(m_Device.waitIdle());

        for (const FrameData &frame_data : m_Frames) {
            m_Device.destroyFence(frame_data.RenderFence);
            m_Device.destroySemaphore(frame_data.SwapchainSemaphore);
            m_Device.destroyCommandPool(frame_data.CommandPool);
        }

        m_Device.destroyFence(m_ImmFence);
        m_Device.destroyCommandPool(m_ImmCommandPool);

        destroyPresentSemaphores();

        DIGNIS_LOG_ENGINE_INFO("Ignis::RenderLayer Released");

        m_pVulkanLayer = nullptr;
    }

    void RenderLayer::onEvent(AEvent &event) {
        EventDispatcher dispatcher(event);

        dispatcher.dispatch<WindowResizeEvent>(this, &RenderLayer::onWindowResizeEvent);
    }

    void RenderLayer::onPreRender() {
        FrameData &frame = getCurrentFrameData();

        DIGNIS_VK_CHECK(m_Device.waitForFences(frame.RenderFence, vk::True, UINT32_MAX));

        {
            auto [result, index] = m_Device.acquireNextImageKHR(
                m_Swapchain,
                UINT32_MAX,
                frame.SwapchainSemaphore);
            if (vk::Result::eSuboptimalKHR == result ||
                vk::Result::eErrorOutOfDateKHR == result) {
                frame.SwapchainImageIndex = UINT32_MAX;
                return;
            }
            DIGNIS_VK_CHECK(result);
            frame.SwapchainImageIndex = index;

            frame.Image       = m_pVulkanLayer->getSwapchainImage(index);
            frame.ImageView   = m_pVulkanLayer->getSwapchainImageView(index);
            frame.ImageExtent = m_pVulkanLayer->getSwapchainExtent();
        }

        DIGNIS_VK_CHECK(m_Device.resetFences(frame.RenderFence));

        const vk::CommandBuffer cmd = frame.CommandBuffer;

        DIGNIS_VK_CHECK(cmd.reset());

        Vulkan::CommandBuffer::BeginOneTimeSubmit(cmd);

        frame.FrameGraphBuilder.clear();

        frame.SwapchainImageID = frame.FrameGraphBuilder.import_image(
            frame.Image,
            frame.ImageView,
            vk::Extent3D{frame.ImageExtent, 1},
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::ePresentSrcKHR);
    }

    void RenderLayer::onRender() {
        if (shouldSkipFrame())
            return;
    }

    void RenderLayer::onPostRender() {
        if (shouldSkipFrame())
            return;

        FrameData &frame = getCurrentFrameData();

        const vk::CommandBuffer cmd = frame.CommandBuffer;

        FrameGraph frame_graph = frame.FrameGraphBuilder.build();
        frame_graph.execute(cmd);

        Vulkan::CommandBuffer::End(cmd);

        const vk::Semaphore frame_present_semaphore = m_PresentSemaphores[frame.SwapchainImageIndex];

        Vulkan::Queue::Submit(
            Vulkan::CommandBuffer::GetSubmitInfo(cmd),
            Vulkan::Semaphore::GetSubmitInfo(
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                frame.SwapchainSemaphore),
            Vulkan::Semaphore::GetSubmitInfo(
                vk::PipelineStageFlagBits2::eAllCommands,
                frame_present_semaphore),
            frame.RenderFence,
            m_GraphicsQueue);

        {
            const vk::Result result = Vulkan::Queue::Present(
                frame_present_semaphore,
                frame.SwapchainImageIndex,
                m_Swapchain,
                m_PresentQueue);

            if (vk::Result::eErrorOutOfDateKHR == result) {
                return;
            }

            if (vk::Result::eSuboptimalKHR != result)
                DIGNIS_VK_CHECK(result);
        }

        m_FrameIndex = (m_FrameIndex + 1) % m_FramesInFlight;
    }

    void RenderLayer::immediateSubmit(fu2::function<void(vk::CommandBuffer cmd)> fn) const {
        DIGNIS_VK_CHECK(m_Device.resetFences(m_ImmFence));
        DIGNIS_VK_CHECK(m_ImmCommandBuffer.reset());

        const vk::CommandBuffer cmd = m_ImmCommandBuffer;

        Vulkan::CommandBuffer::BeginOneTimeSubmit(cmd);
        fn(cmd);
        Vulkan::CommandBuffer::End(cmd);

        Vulkan::Queue::Submit(
            {Vulkan::CommandBuffer::GetSubmitInfo(cmd)},
            {},
            {},
            m_ImmFence,
            m_GraphicsQueue);
        DIGNIS_VK_CHECK(m_Device.waitForFences(m_ImmFence, vk::True, UINT32_MAX));
    }

    bool RenderLayer::onWindowResizeEvent(const WindowResizeEvent &resize_event) {
        const uint32_t width  = resize_event.Width;
        const uint32_t height = resize_event.Height;

        DIGNIS_VK_CHECK(m_Device.waitIdle());

        m_pVulkanLayer->resizeSwapchain(width, height);

        m_SwapchainImageCount = m_pVulkanLayer->getSwapchainImageCount();
        m_SwapchainExtent     = m_pVulkanLayer->getSwapchainExtent();
        m_SwapchainFormat     = m_pVulkanLayer->getSwapchainFormat().format;

        m_Swapchain = m_pVulkanLayer->getSwapchain();

        destroyPresentSemaphores();
        createPresentSemaphores();

        immediateSubmit([this](const vk::CommandBuffer cmd) {
            for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
                Vulkan::Image::TransitionLayout(m_pVulkanLayer->getSwapchainImage(i), vk::ImageLayout::ePresentSrcKHR, cmd);
            }
        });

        return false;
    }

    bool RenderLayer::shouldSkipFrame() const {
        const FrameData &frame_data = m_Frames[m_FrameIndex];
        return UINT32_MAX == frame_data.SwapchainImageIndex;
    }

    RenderLayer::FrameData &RenderLayer::getCurrentFrameData() {
        return m_Frames[m_FrameIndex];
    }

    void RenderLayer::createPresentSemaphores() {
        m_PresentSemaphores.clear();
        m_PresentSemaphores.reserve(m_SwapchainImageCount);
        for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
            vk::Semaphore semaphore = Vulkan::Semaphore::Create(m_Device);
            m_PresentSemaphores.push_back(semaphore);
        }
    }

    void RenderLayer::destroyPresentSemaphores() {
        for (const vk::Semaphore &semaphore : m_PresentSemaphores)
            m_Device.destroySemaphore(semaphore);
        m_PresentSemaphores.clear();
    }
}  // namespace Ignis