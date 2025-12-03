#include <Ignis/Render.hpp>
#include <Ignis/Window.hpp>

namespace Ignis {
    Render *Render::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Render::State Render::s_State{});

    Render &Render::GetRef() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return *s_pInstance;
    }

    void Render::ImmediateSubmit(const fu2::function<void(vk::CommandBuffer)> &fn) {
        GetRef().immediateSubmit(fn);
    }

    void Render::initialize(const Settings &settings) {
        DIGNIS_ASSERT(nullptr == s_pInstance, "Ignis::Render is already initialized.");

        m_FrameIndex     = 0;
        m_FramesInFlight = settings.FramesInFlight;

        m_Frames.clear();
        m_Frames.reserve(m_FramesInFlight);

        for (uint32_t i = 0; i < m_FramesInFlight; i++) {
            FrameData frame_data{};
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

        m_ImmCommandPool   = Vulkan::CreateCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        m_ImmCommandBuffer = Vulkan::AllocatePrimaryCommandBuffer(m_ImmCommandPool);
        m_ImmFence         = Vulkan::CreateFence(vk::FenceCreateFlagBits::eSignaled);

        m_FrameGraph.clear();
        m_GPUScenes.clear();
        m_GPUSceneLookUp.clear();

        Window::GetRef().addListener<WindowResizeEvent>(
            [this](const WindowResizeEvent &event) {
                const auto &[width, height] = event;
                resize(width, height);
                return false;
            },
            typeid(Render));

        s_pInstance = this;
        DIGNIS_LOG_ENGINE_INFO("Ignis::Render Initialized");

        immediateSubmit([](const vk::CommandBuffer command_buffer) {
            Vulkan::BarrierMerger barrier_merger{};
            for (uint32_t i = 0; i < Vulkan::GetSwapchainImageCount(); i++) {
                barrier_merger.put_image_barrier(
                    Vulkan::GetSwapchainImage(i),
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::PipelineStageFlagBits2::eTopOfPipe,
                    vk::AccessFlagBits2::eMemoryWrite,
                    vk::PipelineStageFlagBits2::eAllCommands,
                    vk::AccessFlagBits2::eMemoryRead |
                        vk::AccessFlagBits2::eMemoryWrite);
            }
            barrier_merger.flushBarriers(command_buffer);
        });
    }

    void Render::shutdown() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        Vulkan::WaitDeviceIdle();

        Window::GetRef().removeListener(typeid(Render));

        for (const vk::Semaphore &semaphore : m_PresentSemaphores) {
            Vulkan::DestroySemaphore(semaphore);
        }

        Vulkan::DestroyFence(m_ImmFence);
        Vulkan::DestroyCommandPool(m_ImmCommandPool);

        for (const FrameData &frame_data : m_Frames) {
            Vulkan::DestroyFence(frame_data.RenderFence);
            Vulkan::DestroySemaphore(frame_data.SwapchainSemaphore);
            Vulkan::DestroyCommandPool(frame_data.CommandPool);
        }

        m_Frames.clear();

        DIGNIS_LOG_ENGINE_INFO("Ignis::Render Shutdown");

        s_pInstance = nullptr;
    }

    void Render::immediateSubmit(fu2::function<void(vk::CommandBuffer)> fn) const {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");

        const vk::Fence fence = m_ImmFence;

        const vk::CommandBuffer command_buffer = m_ImmCommandBuffer;

        Vulkan::ResetFences(fence);
        Vulkan::ResetCommandBuffer(command_buffer);
        Vulkan::BeginCommandBuffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, command_buffer);
        fn(command_buffer);
        Vulkan::EndCommandBuffer(command_buffer);

        Vulkan::Submit(
            {Vulkan::GetCommandBufferSubmitInfo(command_buffer)},
            {},
            {},
            fence,
            Vulkan::GetGraphicsQueue());

        Vulkan::WaitForAllFences(fence);
    }

    FrameGraph &Render::getFrameGraph() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return m_FrameGraph;
    }

    Render::FrameData &Render::getCurrentFrameDataRef() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return m_Frames[s_pInstance->m_FrameIndex];
    }

    void Render::resize(const uint32_t width, const uint32_t height) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        Vulkan::WaitDeviceIdle();
        Vulkan::ResizeSwapchain(width, height);

        for (const vk::Semaphore &semaphore : m_PresentSemaphores)
            Vulkan::DestroySemaphore(semaphore);

        m_PresentSemaphores.resize(Vulkan::GetSwapchainImageCount());

        for (vk::Semaphore &semaphore : m_PresentSemaphores)
            semaphore = Vulkan::CreateSemaphore();

        immediateSubmit([](const vk::CommandBuffer command_buffer) {
            Vulkan::BarrierMerger barrier_merger{};
            for (uint32_t i = 0; i < Vulkan::GetSwapchainImageCount(); i++) {
                barrier_merger.put_image_barrier(
                    Vulkan::GetSwapchainImage(i),
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::PipelineStageFlagBits2::eTopOfPipe,
                    vk::AccessFlagBits2::eMemoryWrite,
                    vk::PipelineStageFlagBits2::eAllCommands,
                    vk::AccessFlagBits2::eMemoryRead |
                        vk::AccessFlagBits2::eMemoryWrite);
            }
            barrier_merger.flushBarriers(command_buffer);
        });
    }

    bool Render::beginFrame() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        FrameData &frame_data = m_Frames[m_FrameIndex];

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
            Vulkan::GetSwapchainExtent());

        return true;
    }

    bool Render::endFrame(FrameGraph::Executor &&frame_graph_executor) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");

        const FrameData &frame_data = m_Frames[m_FrameIndex];

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

    IGNIS_IF_DEBUG(Render::State::~State() {
        assert(nullptr == s_pInstance && "Forgot to shutdown Ignis::Render");
    });
}  // namespace Ignis