#include <Ignis/Frame.hpp>

namespace Ignis {
    Frame *Frame::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Frame::State Frame::s_State{});

    Frame &Frame::GetRef() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Frame is not initialized.");
        return *s_pInstance;
    }

    void Frame::initialize(const Settings &settings) {
        DIGNIS_ASSERT(nullptr == s_pInstance, "Ignis::Frame is already initialized.");

        m_FrameIndex     = 0;
        m_FramesInFlight = settings.FramesInFlight;

        m_Frames.clear();
        m_Frames.reserve(m_FramesInFlight);

        for (uint32_t i = 0; i < m_FramesInFlight; i++) {
            Data frame_data{};
            frame_data.CommandPool   = Vulkan::CreateCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
            frame_data.CommandBuffer = Vulkan::AllocatePrimaryCommandBuffer(frame_data.CommandPool);

            frame_data.SwapchainSemaphore = Vulkan::CreateSemaphore();
            frame_data.RenderFence        = Vulkan::CreateFence(vk::FenceCreateFlagBits::eSignaled);

            m_Frames.push_back(frame_data);
        }

        m_PresentSemaphores.resize(Vulkan::GetSwapchainImageCount());

        for (vk::Semaphore &semaphore : m_PresentSemaphores) {
            semaphore = Vulkan::CreateSemaphore();
        }

        m_FrameGraph.clear();

        Window::GetRef().addListener<WindowResizeEvent>(
            [this](const WindowResizeEvent &event) {
                DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Frame is not initialized.");
                const auto &[width, height] = event;
                Vulkan::WaitDeviceIdle();
                Vulkan::GetRef().resizeSwapchain(width, height);

                for (const vk::Semaphore &semaphore : m_PresentSemaphores)
                    Vulkan::DestroySemaphore(semaphore);

                m_PresentSemaphores.resize(Vulkan::GetSwapchainImageCount());

                for (vk::Semaphore &semaphore : m_PresentSemaphores)
                    semaphore = Vulkan::CreateSemaphore();
                return false;
            },
            typeid(Frame));

        s_pInstance = this;
        DIGNIS_LOG_ENGINE_INFO("Ignis::Frame Initialized");
    }

    void Frame::shutdown() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Frame is not initialized.");
        Vulkan::WaitDeviceIdle();

        Window::GetRef().removeListener(typeid(Frame));

        for (const vk::Semaphore &semaphore : m_PresentSemaphores) {
            Vulkan::DestroySemaphore(semaphore);
        }

        for (const Data &frame_data : m_Frames) {
            Vulkan::DestroyFence(frame_data.RenderFence);
            Vulkan::DestroySemaphore(frame_data.SwapchainSemaphore);
            Vulkan::DestroyCommandPool(frame_data.CommandPool);
        }

        m_Frames.clear();

        DIGNIS_LOG_ENGINE_INFO("Ignis::Frame Shutdown");

        s_pInstance = nullptr;
    }

    FrameGraph &Frame::getFrameGraph() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Frame is not initialized.");
        return m_FrameGraph;
    }

    Frame::Data &Frame::getCurrentFrameDataRef() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Frame is not initialized.");
        return m_Frames[s_pInstance->m_FrameIndex];
    }

    bool Frame::begin() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Frame is not initialized.");
        Data &frame_data = m_Frames[m_FrameIndex];

        Vulkan::WaitForAllFences(frame_data.RenderFence);

        auto [result, swapchain_image_index] = Vulkan::AcquireNextImage(frame_data.SwapchainSemaphore);
        if (vk::Result::eErrorOutOfDateKHR == result) {
            return false;
        }
        if (vk::Result::eSuboptimalKHR != result)
            DIGNIS_VK_CHECK(result);

        Vulkan::ResetFences(frame_data.RenderFence);

        frame_data.SwapchainImageIndex = swapchain_image_index;

        m_FrameGraph.beginFrame(
            Vulkan::GetSwapchainImage(swapchain_image_index),
            Vulkan::GetSwapchainImageView(swapchain_image_index),
            Vulkan::GetSwapchainFormat().format,
            Vulkan::GetSwapchainUsageFlags(),
            Vulkan::GetSwapchainExtent());

        return true;
    }

    bool Frame::end(FrameGraph::Executor &&frame_graph_executor) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Frame is not initialized.");

        const Data &frame_data = m_Frames[m_FrameIndex];

        const vk::CommandBuffer command_buffer = frame_data.CommandBuffer;

        DIGNIS_VK_CHECK(command_buffer.reset());

        Vulkan::BeginCommandBuffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, command_buffer);

        FrameGraph::Execute(std::move(frame_graph_executor), command_buffer);

        Vulkan::EndCommandBuffer(command_buffer);

        const vk::Semaphore frame_present_semaphore = m_PresentSemaphores[frame_data.SwapchainImageIndex];

        Vulkan::Submit(
            {Vulkan::GetCommandBufferSubmitInfo(command_buffer)},
            {Vulkan::GetSemaphoreSubmitInfo(
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                frame_data.SwapchainSemaphore)},
            {Vulkan::GetSemaphoreSubmitInfo(
                vk::PipelineStageFlagBits2::eAllCommands,
                frame_present_semaphore)},
            frame_data.RenderFence,
            Vulkan::GetGraphicsQueue());

        const vk::Result result = Vulkan::Present(
            frame_present_semaphore,
            frame_data.SwapchainImageIndex);

        if (vk::Result::eErrorOutOfDateKHR == result)
            return false;

        if (vk::Result::eSuboptimalKHR != result)
            DIGNIS_VK_CHECK(result);

        m_FrameIndex = (m_FrameIndex + 1) % m_FramesInFlight;

        return true;
    }

    IGNIS_IF_DEBUG(Frame::State::~State() {
        assert(nullptr == s_pInstance && "Forgot to shutdown Ignis::Frame");
    });
}  // namespace Ignis