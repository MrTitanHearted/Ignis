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
        logger_settings.EngineLogLevel      = spdlog::level::trace;
        logger_settings.ApplicationLogLevel = spdlog::level::trace;
#else
        logger_settings.EngineLogLevel      = spdlog::level::err;
        logger_settings.ApplicationLogLevel = spdlog::level::warn;
#endif

        const spdlog::sink_ptr editor_sink =
            logger_settings.ApplicationSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                "IGNIS_EDITOR.log", true));
        editor_sink->set_pattern(Logger::GetFileSinkPattern());

        Logger::Initialize(&m_Logger, logger_settings);
        Engine::Initialize(&m_Engine, {
                                          .WindowSettings = Window::Settings{
                                              .Title = "Ignis::Editor",
                                          },
                                      });

        Window::SetIcon("Assets/Icons/IgnisEditor.png");

        m_DeltaTime = 0.0;

        Vulkan::Context::Initialize(&m_VulkanContext, {});

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

        m_Frames.clear();
        m_Frames.reserve(m_FramesInFlight);
        for (uint32_t i = 0; i < m_FramesInFlight; i++) {
            FrameData frame_data{};
            frame_data.CommandPool   = Vulkan::CommandPool::CreateResetCommandBuffer(m_QueueFamilyIndex, m_Device);
            frame_data.CommandBuffer = Vulkan::CommandBuffer::AllocatePrimary(frame_data.CommandPool, m_Device);

            frame_data.SwapchainSemaphore = Vulkan::Semaphore::Create(m_Device);

            frame_data.RenderFence = Vulkan::Fence::CreateSignaled(m_Device);

            m_Frames.push_back(frame_data);
        }

        createRenderSemaphores();

        initializeImmData();
        initializeImGui();

        createViewportImage(1, 1);

        m_ResizeRequested = false;

        immediateSubmit([](const vk::CommandBuffer cmd) {
            for (uint32_t i = 0; i < Vulkan::Context::GetSwapchainImageCount(); i++) {
                Vulkan::Image::TransitionLayout(Vulkan::Context::GetSwapchainImage(i), vk::ImageLayout::ePresentSrcKHR, cmd);
            }
        });

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor initialized");
    }

    void Editor::release() {
        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor released");

        DIGNIS_VK_CHECK(m_Device.waitIdle());

        destroyViewportImage();
        releaseImGui();
        releaseImmData();

        destroyRenderSemaphores();

        for (const auto &frame_data : m_Frames) {
            m_Device.destroyFence(frame_data.RenderFence);
            m_Device.destroySemaphore(frame_data.SwapchainSemaphore);
            m_Device.destroyCommandPool(frame_data.CommandPool);
        }

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

                    if (Window::IsFullscreen()) {
                        Window::MakeWindowed();
                    } else {
                        Window::MakeFullscreen();
                    }
                },
                [this](const WindowResizeEvent &resize_event) {
                    auto [width, height] = resize_event;
                    WindowEvents inner_events;
                    while (width == 0 || height == 0) {
                        Window::WaitEvents(&inner_events);
                        width  = Window::GetWidth();
                        height = Window::GetHeight();
                    }

                    m_ResizeRequested = true;
                });

            if (m_ResizeRequested) {
                const auto [width, height] = Window::GetSize();
                resize(width, height);
            }

            {
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                const ImGuiViewport *viewport = ImGui::GetMainViewport();

                ImGui::SetNextWindowPos(viewport->Pos);
                ImGui::SetNextWindowSize(viewport->Size);
                ImGui::SetNextWindowViewport(viewport->ID);

                constexpr ImGuiWindowFlags host_window_flags =
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_NoNavFocus |
                    ImGuiWindowFlags_NoDocking;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

                ImGui::Begin("Ignis::Editor::DockSpace", nullptr, host_window_flags);
                ImGui::PopStyleVar(3);
                const ImGuiID dockspace_id = ImGui::GetID("Ignis::Editor::DockSpace");
                ImGui::DockSpace(dockspace_id);
                ImGui::End();

                renderImGui();

                ImGui::EndFrame();
                ImGui::Render();

                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            drawFrame();

            m_Timer.stop();
            m_DeltaTime = m_Timer.getElapsedTime();
        }
    }

    void Editor::createRenderSemaphores() {
        const uint32_t swapchain_image_count = Vulkan::Context::GetSwapchainImageCount();
        m_PresentSemaphores.clear();
        m_PresentSemaphores.reserve(swapchain_image_count);
        for (uint32_t i = 0; i < swapchain_image_count; i++) {
            vk::Semaphore present_semaphore = Vulkan::Semaphore::Create(m_Device);
            m_PresentSemaphores.push_back(present_semaphore);
        }
    }

    void Editor::destroyRenderSemaphores() {
        for (const vk::Semaphore &semaphore : m_PresentSemaphores) {
            m_Device.destroySemaphore(semaphore);
        }

        m_PresentSemaphores.clear();
    }

    void Editor::initializeImmData() {
        m_ImmFence = Vulkan::Fence::CreateSignaled(m_Device);

        m_ImmCommandPool   = Vulkan::CommandPool::CreateResetCommandBuffer(m_QueueFamilyIndex, m_Device);
        m_ImmCommandBuffer = Vulkan::CommandBuffer::AllocatePrimary(m_ImmCommandPool, m_Device);
    }

    void Editor::releaseImmData() {
        m_Device.destroyCommandPool(m_ImmCommandPool);
        m_Device.destroyFence(m_ImmFence);

        m_ImmCommandBuffer = nullptr;
    }

    void Editor::initializeImGui() {
        m_ImGuiDescriptorPool = Vulkan::DescriptorPool::Create(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            100,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 100},
            },
            m_Device);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |=
            ImGuiConfigFlags_DockingEnable |
            ImGuiConfigFlags_ViewportsEnable;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(Window::Get(), true);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance            = Vulkan::Context::GetInstance();
        init_info.PhysicalDevice      = Vulkan::Context::GetPhysicalDevice();
        init_info.Device              = m_Device;
        init_info.QueueFamily         = m_QueueFamilyIndex;
        init_info.Queue               = m_GraphicsQueue;
        init_info.DescriptorPool      = m_ImGuiDescriptorPool;
        init_info.MinImageCount       = 3;
        init_info.ImageCount          = Vulkan::Context::GetSwapchainImageCount();
        init_info.UseDynamicRendering = true;
        init_info.ApiVersion          = VK_API_VERSION_1_4;

        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo =
            vk::PipelineRenderingCreateInfo{}
                .setColorAttachmentFormats({m_SwapchainFormat});
        init_info.PipelineInfoForViewports = init_info.PipelineInfoMain;

        ImGui_ImplVulkan_Init(&init_info);

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.ConfigWindowsResizeFromEdges = false;
    }

    void Editor::releaseImGui() {
        if (const ImGuiIO &io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::DestroyPlatformWindows();
        }
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_Device.destroyDescriptorPool(m_ImGuiDescriptorPool);

        m_ImGuiDescriptorPool = nullptr;
    }

    void Editor::createViewportImage(
        const uint32_t width,
        const uint32_t height) {
        m_ViewportFormat = vk::Format::eR32G32B32A32Sfloat;

        m_ViewportExtent
            .setWidth(width)
            .setHeight(height);

        m_ViewportImage =
            Vulkan::Image::Allocate2D(
                vma::MemoryUsage::eGpuOnly,
                m_ViewportFormat,
                vk::ImageUsageFlagBits::eColorAttachment |
                    vk::ImageUsageFlagBits::eSampled |
                    vk::ImageUsageFlagBits::eTransferSrc |
                    vk::ImageUsageFlagBits::eTransferDst,
                m_ViewportExtent,
                m_VmaAllocator);

        m_ViewportImageView = Vulkan::ImageView::CreateColor2D(m_ViewportFormat, m_ViewportImage.Image, m_Device);

        m_ViewportImageSampler = Vulkan::Sampler::Create({}, m_Device);

        immediateSubmit([this](const vk::CommandBuffer cmd) {
            Vulkan::Image::TransitionLayout(m_ViewportImage.Image, vk::ImageLayout::eShaderReadOnlyOptimal, cmd);
        });

        m_ViewportImageDescriptor = ImGui_ImplVulkan_AddTexture(
            m_ViewportImageSampler,
            m_ViewportImageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void Editor::destroyViewportImage() {
        ImGui_ImplVulkan_RemoveTexture(m_ViewportImageDescriptor);

        m_Device.destroySampler(m_ViewportImageSampler);
        m_Device.destroyImageView(m_ViewportImageView);

        Vulkan::Image::Destroy(m_ViewportImage, m_VmaAllocator);

        m_ViewportImageView = nullptr;
    }

    void Editor::renderImGui() {
        ImGui::Begin("Ignis::Editor::Toolbox");

        ImGui::ColorEdit3("Viewport Clear Color", &m_ViewportClearColor.r);

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Ignis::Editor::Viewport");
        ImGui::PopStyleVar(2);

        const ImVec2 viewport_size = ImGui::GetContentRegionAvail();

        if (static_cast<uint32_t>(viewport_size.x) != m_ViewportExtent.width ||
            static_cast<uint32_t>(viewport_size.y) != m_ViewportExtent.height) {
            DIGNIS_VK_CHECK(m_Device.waitIdle())

            destroyViewportImage();
            createViewportImage(viewport_size.x, viewport_size.y);
        }

        ImGui::Image(static_cast<ImTextureRef>(static_cast<const VkDescriptorSet &>(m_ViewportImageDescriptor)), viewport_size);

        ImGui::End();
    }

    void Editor::drawFrame() {
        const FrameData &frame = getCurrentFrameData();

        DIGNIS_VK_CHECK(m_Device.waitForFences(frame.RenderFence, vk::True, UINT32_MAX));

        uint32_t swapchain_image_index = 0;
        {
            auto [result, index] = m_Device.acquireNextImageKHR(
                m_Swapchain,
                UINT32_MAX,
                frame.SwapchainSemaphore);
            if (vk::Result::eSuboptimalKHR == result ||
                vk::Result::eErrorOutOfDateKHR == result) {
                m_ResizeRequested = true;
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

        const vk::Semaphore frame_present_semaphore = m_PresentSemaphores[swapchain_image_index];

        Vulkan::CommandBuffer::BeginOneTimeSubmit(cmd);

        Vulkan::Image::TransitionLayout(m_ViewportImage.Image, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal, cmd);
        drawGeometry(m_ViewportImageView, m_ViewportExtent, cmd);
        Vulkan::Image::TransitionLayout(m_ViewportImage.Image, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, cmd);

        Vulkan::Image::TransitionLayout(frame_image, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eColorAttachmentOptimal, cmd);
        drawImGui(frame_view, m_SwapchainExtent, cmd);
        Vulkan::Image::TransitionLayout(frame_image, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, cmd);

        Vulkan::CommandBuffer::End(cmd);

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
                swapchain_image_index,
                m_Swapchain,
                m_PresentQueue);

            if (vk::Result::eErrorOutOfDateKHR == result) {
                m_ResizeRequested = true;
                return;
            }

            if (vk::Result::eSuboptimalKHR != result)
                DIGNIS_VK_CHECK(result);
        }

        m_FrameIndex = (m_FrameIndex + 1) % m_FramesInFlight;
    }

    void Editor::drawGeometry(
        const vk::ImageView     target,
        const vk::Extent2D     &extent,
        const vk::CommandBuffer cmd) const {
        (void)m_Device;
        vk::Viewport viewport{};
        viewport
            .setX(0)
            .setY(extent.height)
            .setWidth(static_cast<float>(extent.width))
            .setHeight(-static_cast<float>(extent.height))
            .setMinDepth(0.0f)
            .setMaxDepth(1.0f);
        vk::Rect2D scissor{};
        scissor
            .setOffset(vk::Offset2D{0, 0})
            .setExtent(extent);

        const vk::ClearValue clear_value = vk::ClearColorValue{
            m_ViewportClearColor.r,
            m_ViewportClearColor.g,
            m_ViewportClearColor.b,
            1.0f,
        };
        Vulkan::RenderPass::Begin(
            extent,
            {Vulkan::RenderPass::GetAttachmentInfo(
                target,
                vk::ImageLayout::eColorAttachmentOptimal,
                clear_value)},
            cmd);

        cmd.setViewport(0, viewport);
        cmd.setScissor(0, scissor);

        Vulkan::RenderPass::End(cmd);
    }

    void Editor::drawImGui(
        const vk::ImageView     target,
        const vk::Extent2D     &extent,
        const vk::CommandBuffer cmd) const {
        (void)extent;
        Vulkan::RenderPass::Begin(
            m_SwapchainExtent,
            {Vulkan::RenderPass::GetAttachmentInfo(target, vk::ImageLayout::eColorAttachmentOptimal)},
            cmd);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        Vulkan::RenderPass::End(cmd);
    }

    void Editor::resize(const uint32_t width, const uint32_t height) {
        DIGNIS_VK_CHECK(m_Device.waitIdle());

        Vulkan::Context::ResizeSwapchain(width, height);
        m_Swapchain       = Vulkan::Context::GetSwapchain();
        m_SwapchainExtent = Vulkan::Context::GetSwapchainExtent();

        immediateSubmit([](const vk::CommandBuffer cmd) {
            for (uint32_t i = 0; i < Vulkan::Context::GetSwapchainImageCount(); i++) {
                Vulkan::Image::TransitionLayout(Vulkan::Context::GetSwapchainImage(i), vk::ImageLayout::ePresentSrcKHR, cmd);
            }
        });

        destroyRenderSemaphores();
        createRenderSemaphores();

        m_ResizeRequested = false;
    }

    void Editor::immediateSubmit(fu2::function<void(vk::CommandBuffer cmd)> &&function) const {
        DIGNIS_VK_CHECK(m_Device.resetFences(m_ImmFence));
        DIGNIS_VK_CHECK(m_ImmCommandBuffer.reset());

        const vk::CommandBuffer cmd = m_ImmCommandBuffer;

        Vulkan::CommandBuffer::BeginOneTimeSubmit(cmd);
        function(cmd);
        Vulkan::CommandBuffer::End(cmd);

        Vulkan::Queue::Submit(
            {Vulkan::CommandBuffer::GetSubmitInfo(cmd)},
            {},
            {},
            m_ImmFence,
            m_GraphicsQueue);
        DIGNIS_VK_CHECK(m_Device.waitForFences(m_ImmFence, vk::True, UINT32_MAX));
    }

    Editor::FrameData &Editor::getCurrentFrameData() {
        return m_Frames[m_FrameIndex];
    }
}  // namespace Ignis