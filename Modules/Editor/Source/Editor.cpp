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
        Engine::Initialize(&m_Engine, {});

        m_DeltaTime = 0.0;

        Vulkan::Context::Initialize(
            &m_VulkanContext,
            {
                .PreferredImageCount = 3,
                .SurfaceUsageFlags   = vk::ImageUsageFlagBits::eStorage,
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

        m_StopRendering   = false;
        m_ResizeRequested = false;

        initializeImmData();
        initializeImGuiData();

        initializeDrawData();

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor initialized");
    }

    void Editor::release() {
        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor released");

        DIGNIS_VK_CHECK(m_Device.waitIdle());

        releaseDrawData();

        releaseImGuiData();
        releaseImmData();

        releaseRenderSemaphores();

        for (const auto &frame_data : m_Frames) {
            m_Device.destroyFence(frame_data.RenderFence);
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
                [this](const WindowResizeEvent &) {
                    // resize(resize_event.Width, resize_event.Height);
                    m_ResizeRequested = true;
                },
                [this](const WindowRestoreEvent &) {
                    m_StopRendering = false;
                },
                [this](const WindowMinimizeEvent &) {
                    m_StopRendering = true;
                });

            if (m_StopRendering) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            if (m_ResizeRequested) {
                const auto [width, height] = Window::GetSize();
                resize(width, height);
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            renderImGui();

            ImGui::EndFrame();
            ImGui::Render();

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            drawFrame();

            m_Timer.stop();
            m_DeltaTime = m_Timer.getElapsedTime();
        }
    }

    void Editor::renderImGui() {
        ImGui::ShowDemoWindow();
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

        const vk::Image     frame_image             = Vulkan::Context::GetSwapchainImage(swapchain_image_index);
        const vk::ImageView frame_view              = Vulkan::Context::GetSwapchainImageView(swapchain_image_index);
        const vk::Semaphore frame_present_semaphore = m_PresentSemaphores[swapchain_image_index];

        Vulkan::CommandBuffer::BeginOneTimeSubmit(cmd);

        Vulkan::Image::TransitionLayout(m_DrawImage.Image, vk::ImageLayout::eGeneral, cmd);

        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_GradientPipeline);
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            m_GradientPipelineLayout,
            0,
            {m_DrawImageDescriptor},
            {});
        cmd.dispatch(
            std::ceil(m_SwapchainExtent.width / 16.0),
            std::ceil(m_SwapchainExtent.height / 16.0),
            1);

        Vulkan::Image::TransitionLayout(m_DrawImage.Image, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal, cmd);
        Vulkan::Image::TransitionLayout(frame_image, vk::ImageLayout::eTransferDstOptimal, cmd);

        Vulkan::CommandBuffer::CopyImageToImage(
            m_DrawImage.Image,
            frame_image,
            vk::Extent3D{m_SwapchainExtent, 1},
            vk::Extent3D{m_SwapchainExtent, 1},
            cmd);

        Vulkan::Image::TransitionLayout(frame_image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal, cmd);

        drawImGui(frame_view, cmd);

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

    void Editor::drawImGui(
        const vk::ImageView     target,
        const vk::CommandBuffer cmd) {
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
        m_Swapchain = Vulkan::Context::GetSwapchain();
        m_SwapchainExtent
            .setWidth(width)
            .setHeight(height);

        releaseRenderSemaphores();
        createRenderSemaphores();

        releaseDrawImage();
        initializeDrawImage();

        m_ResizeRequested = false;
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

    void Editor::releaseRenderSemaphores() {
        for (const vk::Semaphore &semaphore : m_PresentSemaphores) {
            m_Device.destroySemaphore(semaphore);
        }

        m_PresentSemaphores.clear();
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

    void Editor::initializeImGuiData() {
        m_ImGuiDescriptorPool = Vulkan::DescriptorPool::Create(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            1000,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eSampler, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, 1000},
                vk::DescriptorPoolSize{vk::DescriptorType::eInputAttachment, 1000},
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
        init_info.Instance                                     = Vulkan::Context::GetInstance();
        init_info.PhysicalDevice                               = Vulkan::Context::GetPhysicalDevice();
        init_info.Device                                       = m_Device;
        init_info.QueueFamily                                  = m_QueueFamilyIndex;
        init_info.Queue                                        = m_GraphicsQueue;
        init_info.DescriptorPool                               = m_ImGuiDescriptorPool;
        init_info.MinImageCount                                = 3;
        init_info.ImageCount                                   = Vulkan::Context::GetSwapchainImageCount();
        init_info.UseDynamicRendering                          = true;
        init_info.ApiVersion                                   = VK_API_VERSION_1_4;
        init_info.PipelineInfoMain.MSAASamples                 = VK_SAMPLE_COUNT_1_BIT;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{}
                                                                     .setColorAttachmentFormats({m_SwapchainFormat});
        init_info.PipelineInfoForViewports = init_info.PipelineInfoMain;

        ImGui_ImplVulkan_Init(&init_info);

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.ConfigWindowsResizeFromEdges = false;
    }

    void Editor::releaseImGuiData() {
        if (const ImGuiIO &io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::DestroyPlatformWindows();
        }
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_Device.destroyDescriptorPool(m_ImGuiDescriptorPool);

        m_ImGuiDescriptorPool = nullptr;
    }

    void Editor::initializeDrawData() {
        const FileAsset shader_source_file_asset =
            FileAsset::BinaryFromPath("Assets/Shaders/Compute.spv").value();

        const std::string_view shader_source = shader_source_file_asset.getContent();
        std::vector<uint32_t>  shader_code{};
        shader_code.resize(shader_source.size() / sizeof(uint32_t));
        memcpy(shader_code.data(), shader_source.data(), shader_source.size());

        m_DescriptorPool = Vulkan::DescriptorPool::Create(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            1,
            {vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 1000}},
            m_Device);

        m_ComputeShaderModule = Vulkan::Shader::CreateModuleFromSPV(shader_code, m_Device);

        m_DrawImageDescriptorLayout = Vulkan::DescriptorLayout::Builder()
                                          .addStorageImage(
                                              0,
                                              vk::ShaderStageFlagBits::eCompute)
                                          .build(m_Device);

        m_DrawImageDescriptor = Vulkan::DescriptorSet::Allocate(
            m_DrawImageDescriptorLayout,
            m_DescriptorPool,
            m_Device);

        m_GradientPipelineLayout = Vulkan::PipelineLayout::Create(
            {m_DrawImageDescriptorLayout},
            m_Device);
        m_GradientPipeline = Vulkan::ComputePipeline::Create(
            "computeMain",
            m_ComputeShaderModule,
            m_GradientPipelineLayout,
            m_Device);

        initializeDrawImage();
    }

    void Editor::releaseDrawData() {
        releaseDrawImage();

        m_Device.destroyPipeline(m_GradientPipeline);
        m_Device.destroyPipelineLayout(m_GradientPipelineLayout);
        m_Device.destroyDescriptorSetLayout(m_DrawImageDescriptorLayout);
        m_Device.destroyShaderModule(m_ComputeShaderModule);
        m_Device.destroyDescriptorPool(m_DescriptorPool);
    }

    void Editor::initializeDrawImage() {
        m_DrawImage = Vulkan::Image::Allocate(
            {},
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eStorage,
            m_SwapchainExtent,
            m_VmaAllocator);

        m_DrawImageView = Vulkan::ImageView::CreateColor2D(m_DrawImage.Format, m_DrawImage.Image, m_Device);

        Vulkan::DescriptorSet::Writer()
            .addStorageImage(0, m_DrawImageView, vk::ImageLayout::eGeneral)
            .write(m_DrawImageDescriptor, m_Device);
    }

    void Editor::releaseDrawImage() {
        m_Device.destroyImageView(m_DrawImageView);
        Vulkan::Image::Destroy(m_DrawImage, m_VmaAllocator);

        m_DrawImageView = nullptr;
    }

    Editor::FrameData &Editor::getCurrentFrameData() {
        return m_Frames[m_FrameIndex];
    }
}  // namespace Ignis