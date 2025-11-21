#include <Editor.hpp>

#include <spdlog/sinks/basic_file_sink.h>

namespace Ignis {
    void Editor::initialize(
        const int32_t argc,
        const char  **argv) {
        (void)argc;
        (void)argv;

        Logger::Settings logger_settings{};
        logger_settings.ApplicationLogName = "EDITOR";
#ifdef IGNIS_BUILD_TYPE_DEBUG
        logger_settings.EngineLogLevel = spdlog::level::trace;
#endif

        const spdlog::sink_ptr editor_sink =
            logger_settings.ApplicationSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                "IGNIS_EDITOR.log", true));
        editor_sink->set_pattern(Logger::GetFileSinkPattern());

        Logger::Initialize(&m_Logger, logger_settings);
        Engine::Initialize(&m_Engine, {});

        m_DeltaTime = 0.0;

        Vulkan::Context::Initialize(
            &m_VulkanContext,
            {
                .PreferredImageCount = 3,
            });

        m_Device           = Vulkan::Context::GetDevice();
        m_Surface          = Vulkan::Context::GetSurface();
        m_QueueFamilyIndex = Vulkan::Context::GetQueueFamilyIndex();
        m_GraphicsQueue    = Vulkan::Context::GetGraphicsQueue();
        m_ComputeQueue     = Vulkan::Context::GetComputeQueue();
        m_PresentQueue     = Vulkan::Context::GetPresentQueue();
        m_Swapchain        = Vulkan::Context::GetSwapchain();
        m_SwapchainFormat  = Vulkan::Context::GetSwapchainFormat().format;
        m_SwapchainExtent  = Vulkan::Context::GetSwapchainExtent();
        m_VmaAllocator     = Vulkan::Context::GetVmaAllocator();

        m_FrameIndex     = 0;
        m_FramesInFlight = Vulkan::Context::GetSwapchainImageCount();

        m_Frames.reserve(m_FramesInFlight);
        for (uint32_t i = 0; i < m_FramesInFlight; i++) {
            FrameData frame_data{};
            frame_data.CommandPool   = Vulkan::CommandPool::CreateResetCommandBuffer(m_QueueFamilyIndex, m_Device);
            frame_data.CommandBuffer = Vulkan::CommandBuffer::AllocatePrimary(frame_data.CommandPool, m_Device);

            frame_data.SwapchainSemaphore = Vulkan::Semaphore::Create(m_Device);
            frame_data.RenderSemaphore    = Vulkan::Semaphore::Create(m_Device);

            frame_data.RenderFence = Vulkan::Fence::CreateSignaled(m_Device);

            m_Frames.push_back(frame_data);
        }

        m_StopRendering = false;

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor initialized");
    }

    void Editor::release() {
        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor released");

        DIGNIS_VK_CHECK(m_Device.waitIdle());

        for (const auto &frame_data : m_Frames) {
            m_Device.destroyFence(frame_data.RenderFence);
            m_Device.destroySemaphore(frame_data.RenderSemaphore);
            m_Device.destroySemaphore(frame_data.SwapchainSemaphore);
            m_Device.destroyCommandPool(frame_data.CommandPool);
        }

        m_Frames.clear();

        m_WindowEvents.clear();

        Vulkan::Context::Shutdown();

        Engine::Shutdown();
        Logger::Shutdown();
    }

    void Editor::run() {
        Engine::Start();
        while (Engine::IsRunning()) {
            m_Timer.start();
            m_WindowEvents.clear();
            Window::PollEvents(&m_WindowEvents);

            m_WindowEvents.visit(
                [](const WindowCloseEvent &) {
                    Window::Close();
                    Engine::Stop();
                },
                [](const WindowKeyEvent &key_event) {
                    if (KeyAction::ePress != key_event.Action)
                        return;

                    if (KeyCode::eF != key_event.Key)
                        return;

                    if (Window::IsFullscreen())
                        Window::MakeWindowed();
                    else
                        Window::MakeFullscreen();
                },
                // [this](const WindowResizeEvent &resize_event) {
                //     // resize(resize_event.Width, resize_event.Height);
                // },
                [this](const WindowRestoreEvent &) {
                    m_StopRendering = false;
                },
                [this](const WindowMinimizeEvent &) {
                    m_StopRendering = true;
                });

            if (!m_StopRendering)
                drawFrame();

            m_Timer.stop();

            m_DeltaTime = m_Timer.getElapsedTime();
        }
    }

    void Editor::drawFrame() {
        // const FrameData &frame = getCurrentFrameData();
        const FrameData &frame = getCurrentFrameData();

        DIGNIS_VK_CHECK(m_Device.waitForFences(frame.RenderFence, vk::True, UINT32_MAX));

        uint32_t swapchain_image_index = 0;
        {
            auto [result, index] = m_Device.acquireNextImageKHR(
                m_Swapchain,
                UINT32_MAX,
                frame.SwapchainSemaphore,
                nullptr);
            if (vk::Result::eSuboptimalKHR == result ||
                vk::Result::eErrorOutOfDateKHR == result) {
                const auto [width, height] = Window::GetSize();
                resize(width, height);
                return;
            }
            DIGNIS_VK_CHECK(result);
            swapchain_image_index = index;
        }

        DIGNIS_VK_CHECK(m_Device.resetFences(frame.RenderFence));

        const vk::CommandBuffer cmd = frame.CommandBuffer;

        DIGNIS_VK_CHECK(cmd.reset());

        const vk::Image     frame_image = Vulkan::Context::GetSwapchainImage(swapchain_image_index);
        const vk::ImageView frame_view  = Vulkan::Context::GetSwapchainImageView(swapchain_image_index);

        Vulkan::CommandBuffer::BeginOneTimeSubmit(cmd);

        Vulkan::Image::TransitionLayout(frame_image, vk::ImageLayout::eColorAttachmentOptimal, cmd);

        constexpr vk::ClearColorValue clear_color_value{0.2f, 0.3f, 0.3f, 1.0f};

        Vulkan::RenderPass::Begin(
            m_SwapchainExtent,
            Vulkan::RenderPass::GetAttachmentInfo(
                frame_view,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ClearValue{clear_color_value}),
            cmd);

        Vulkan::RenderPass::End(cmd);

        Vulkan::Image::TransitionLayout(frame_image, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, cmd);

        Vulkan::CommandBuffer::End(cmd);

        Vulkan::Queue::Submit(
            Vulkan::CommandBuffer::GetSubmitInfo(cmd),
            Vulkan::Semaphore::GetSubmitInfo(
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                frame.SwapchainSemaphore),
            Vulkan::Semaphore::GetSubmitInfo(
                vk::PipelineStageFlagBits2::eAllCommands,
                frame.RenderSemaphore),
            frame.RenderFence,
            m_GraphicsQueue);

        {
            const vk::Result result = Vulkan::Queue::Present(
                frame.RenderSemaphore,
                swapchain_image_index,
                m_Swapchain,
                m_PresentQueue);

            if (vk::Result::eSuboptimalKHR == result ||
                vk::Result::eErrorOutOfDateKHR == result) {
                const auto [width, height] = Window::GetSize();
                resize(width, height);
                return;
            }
            DIGNIS_VK_CHECK(result);
        }

        m_FrameIndex++;

        if (m_FrameIndex == m_FramesInFlight) {
            m_FrameIndex = 0;
        }
    }

    void Editor::resize(const uint32_t width, const uint32_t height) {
        Vulkan::Context::ResizeSwapchain(width, height);
        m_Swapchain = Vulkan::Context::GetSwapchain();
        m_SwapchainExtent
            .setWidth(width)
            .setHeight(height);
    }

    Editor::FrameData &Editor::getCurrentFrameData() {
        return m_Frames[m_FrameIndex];
    }
}  // namespace Ignis