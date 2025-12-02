#include <Ignis/Render.hpp>
#include <Ignis/Window.hpp>

namespace Ignis {
    Render *Render::s_pInstance = nullptr;

    IGNIS_IF_DEBUG(Render::State Render::s_State{});

    void Render::Initialize(Render *render, const Settings &settings) {
        DIGNIS_ASSERT(nullptr == s_pInstance, "Ignis::Render is already initialized.");

        s_pInstance = render;

        s_pInstance->m_FrameIndex     = 0;
        s_pInstance->m_FramesInFlight = settings.FramesInFlight;

        s_pInstance->m_Frames.clear();
        s_pInstance->m_Frames.reserve(s_pInstance->m_FramesInFlight);

        for (uint32_t i = 0; i < s_pInstance->m_FramesInFlight; i++) {
            FrameData frame_data{};
            frame_data.CommandPool   = Vulkan::CreateCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
            frame_data.CommandBuffer = Vulkan::AllocatePrimaryCommandBuffer(frame_data.CommandPool);

            frame_data.SwapchainSempahore = Vulkan::CreateSemaphore();
            frame_data.RenderFence        = Vulkan::CreateFence(vk::FenceCreateFlagBits::eSignaled);

            s_pInstance->m_Frames.push_back(frame_data);
        }

        s_pInstance->m_PresentSemaphores.resize(Vulkan::GetSwapchainImageCount());

        for (vk::Semaphore &semaphore : s_pInstance->m_PresentSemaphores) {
            semaphore = Vulkan::CreateSemaphore();
        }

        s_pInstance->m_ImmCommandPool   = Vulkan::CreateCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        s_pInstance->m_ImmCommandBuffer = Vulkan::AllocatePrimaryCommandBuffer(s_pInstance->m_ImmCommandPool);
        s_pInstance->m_ImmFence         = Vulkan::CreateFence(vk::FenceCreateFlagBits::eSignaled);

        ImmediateSubmit([](const vk::CommandBuffer command_buffer) {
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

        s_pInstance->m_RenderPipelines.clear();
        s_pInstance->m_RenderPipelineLookUp.clear();

        Window::Get().addListener<WindowResizeEvent>(
            [](const WindowResizeEvent &event) {
                const auto &[width, height] = event;
                Resize(width, height);
                return false;
            },
            typeid(Render));
        DIGNIS_LOG_ENGINE_INFO("Ignis::Render Initialized");
    }

    void Render::Shutdown() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        DIGNIS_VK_CHECK(Vulkan::GetDevice().waitIdle());

        Window::Get().removeListener(typeid(Render));

        for (const vk::Semaphore &semaphore : s_pInstance->m_PresentSemaphores) {
            Vulkan::DestroySemaphore(semaphore);
        }

        Vulkan::DestroyFence(s_pInstance->m_ImmFence);
        Vulkan::DestroyCommandPool(s_pInstance->m_ImmCommandPool);

        for (const FrameData &frame_data : s_pInstance->m_Frames) {
            Vulkan::DestroyFence(frame_data.RenderFence);
            Vulkan::DestroySemaphore(frame_data.SwapchainSempahore);
            Vulkan::DestroyCommandPool(frame_data.CommandPool);
        }

        DIGNIS_LOG_ENGINE_INFO("Ignis::Render Shutdown");

        s_pInstance = nullptr;
    }

    void Render::ImmediateSubmit(fu2::function<void(vk::CommandBuffer)> fn) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");

        const vk::Fence fence = s_pInstance->m_ImmFence;

        const vk::CommandBuffer command_buffer = s_pInstance->m_ImmCommandBuffer;

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

    Render::FrameData &Render::GetCurrentFrameDataRef() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->m_Frames[s_pInstance->m_FrameIndex];
    }

    const Render::FrameData &Render::GetCurrentFrameDataConstRef() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        return s_pInstance->m_Frames[s_pInstance->m_FrameIndex];
    }

    void Render::Resize(const uint32_t width, const uint32_t height) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        DIGNIS_VK_CHECK(Vulkan::GetDevice().waitIdle());

        Vulkan::ResizeSwapchain(width, height);

        for (const vk::Semaphore &semaphore : s_pInstance->m_PresentSemaphores)
            Vulkan::DestroySemaphore(semaphore);

        s_pInstance->m_PresentSemaphores.resize(Vulkan::GetSwapchainImageCount());

        for (vk::Semaphore &semaphore : s_pInstance->m_PresentSemaphores)
            semaphore = Vulkan::CreateSemaphore();

        ImmediateSubmit([](const vk::CommandBuffer command_buffer) {
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

    std::optional<Render::FrameImage> Render::BeginFrame() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");
        FrameData &frame_data = GetCurrentFrameDataRef();

        Vulkan::WaitForAllFences(frame_data.RenderFence);

        auto [result, swapchain_image_index] = Vulkan::AcquireNextImage(frame_data.SwapchainSempahore);
        if (vk::Result::eErrorOutOfDateKHR == result) {
            return std::nullopt;
        }
        if (vk::Result::eSuboptimalKHR != result)
            DIGNIS_VK_CHECK(result);

        Vulkan::ResetFences(frame_data.RenderFence);

        frame_data.SwapchainImageIndex = swapchain_image_index;

        return FrameImage{
            Vulkan::GetSwapchainImage(swapchain_image_index),
            Vulkan::GetSwapchainImageView(swapchain_image_index),
            Vulkan::GetSwapchainExtent(),
        };
    }

    bool Render::EndFrame(FrameGraph::Executor &&frame_graph_executor) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Render is not initialized.");

        const FrameData &frame_data = GetCurrentFrameDataRef();

        const vk::CommandBuffer command_buffer = frame_data.CommandBuffer;

        DIGNIS_VK_CHECK(command_buffer.reset());

        Vulkan::BeginCommandBuffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, command_buffer);

        FrameGraph::Execute(std::move(frame_graph_executor), command_buffer);

        Vulkan::EndCommandBuffer(command_buffer);

        const vk::Semaphore frame_present_semaphore = s_pInstance->m_PresentSemaphores[frame_data.SwapchainImageIndex];

        Vulkan::Submit(
            {Vulkan::GetCommandBufferSubmitInfo(command_buffer)},
            {Vulkan::GetSemaphoreSubmitInfo(
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                frame_data.SwapchainSempahore)},
            {Vulkan::GetSemaphoreSubmitInfo(
                vk::PipelineStageFlagBits2::eAllCommands,
                frame_present_semaphore)},
            frame_data.RenderFence,
            Vulkan::GetGraphicsQueue());

        {
            const vk::Result result = Vulkan::Present(
                frame_present_semaphore,
                frame_data.SwapchainImageIndex);

            if (vk::Result::eErrorOutOfDateKHR == result)
                return false;

            if (vk::Result::eSuboptimalKHR != result)
                DIGNIS_VK_CHECK(result);
        }

        s_pInstance->m_FrameIndex = (s_pInstance->m_FrameIndex + 1) % s_pInstance->m_FramesInFlight;

        return true;
    }

    IGNIS_IF_DEBUG(Render::State::~State() {
        assert(nullptr == s_pInstance && "Forgot to shutdown Ignis::Render");
    });
}  // namespace Ignis