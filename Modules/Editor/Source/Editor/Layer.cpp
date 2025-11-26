#include <Ignis/Editor/Layer.hpp>

namespace Ignis {
    EditorLayer::EditorLayer(
        VulkanLayer    *vulkan_layer,
        RenderLayer    *render_layer,
        const Settings &settings) {
        DIGNIS_ASSERT(nullptr != vulkan_layer);
        DIGNIS_ASSERT(nullptr != render_layer);

        m_pVulkanLayer = vulkan_layer;
        m_pRenderLayer = render_layer;

        m_Device           = m_pVulkanLayer->getDevice();
        m_QueueFamilyIndex = m_pVulkanLayer->getQueueFamilyIndex();
        m_GraphicsQueue    = m_pVulkanLayer->getGraphicsQueue();
        m_ComputeQueue     = m_pVulkanLayer->getComputeQueue();
        m_PresentQueue     = m_pVulkanLayer->getPresentQueue();

        m_VmaAllocator = m_pVulkanLayer->getVmaAllocator();

        initializeImGui();

        createViewportImage(1, 1);

        DIGNIS_LOG_APPLICATION_INFO("Ignis::EditorLayer Initialized");
    }

    EditorLayer::~EditorLayer() {
        DIGNIS_LOG_APPLICATION_INFO("Ignis::EditorLayer Released");

        DIGNIS_VK_CHECK(m_Device.waitIdle());

        destroyViewportImage();
        releaseImGui();
    }

    void EditorLayer::onEvent(AEvent &event) {
        EventDispatcher dispatcher(event);

        dispatcher.dispatch<WindowCloseEvent>(this, &EditorLayer::onWindowCloseEvent);
        dispatcher.dispatch<WindowKeyEvent>(this, &EditorLayer::onWindowKeyEvent);
    }

    void EditorLayer::onPreRender() {
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
        const ImGuiID dock_space_id = ImGui::GetID("Ignis::Editor::DockSpace");
        ImGui::DockSpace(dock_space_id);
        ImGui::End();

        renderImGui();

        ImGui::EndFrame();
        ImGui::Render();

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    void EditorLayer::onRender() {
        if (m_pRenderLayer->shouldSkipFrame())
            return;

        RenderLayer::FrameData &frame = m_pRenderLayer->getCurrentFrameData();

        const FrameGraph::ImageID viewport_image_id = frame.FrameGraphBuilder.import_image(
            m_ViewportImage.Image,
            m_ViewportImageView,
            m_ViewportImage.Extent,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        frame.FrameGraphBuilder
            .add_render_pass("Viewport pass")
            .set_color_attachment(FrameGraph::Attachment{
                viewport_image_id,
                vk::ClearValue{}.setColor({m_ViewportClearColor.r, m_ViewportClearColor.g, m_ViewportClearColor.b, 1.0f}),
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            })
            .execute([](const vk::CommandBuffer command_buffer) {

            });

        frame.FrameGraphBuilder
            .add_render_pass("ImGui pass")
            .read_images({FrameGraph::ImageInfo{viewport_image_id, vk::PipelineStageFlagBits2::eFragmentShader}})
            .set_color_attachment(FrameGraph::Attachment{
                frame.SwapchainImageID,
                vk::ClearValue{},
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore,
            })
            .execute([](const vk::CommandBuffer command_buffer) {
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
            });
    }

    void EditorLayer::initializeImGui() {
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

        const WindowLayer *window_layer = Engine::Get().getLayer<WindowLayer>();

        ImGui_ImplGlfw_InitForVulkan(window_layer->get(), true);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance            = m_pVulkanLayer->getInstance();
        init_info.PhysicalDevice      = m_pVulkanLayer->getPhysicalDevice();
        init_info.Device              = m_Device;
        init_info.QueueFamily         = m_QueueFamilyIndex;
        init_info.Queue               = m_GraphicsQueue;
        init_info.DescriptorPool      = m_ImGuiDescriptorPool;
        init_info.MinImageCount       = m_pVulkanLayer->getSwapchainMinImageCount();
        init_info.ImageCount          = m_pVulkanLayer->getSwapchainImageCount();
        init_info.UseDynamicRendering = true;
        init_info.ApiVersion          = VK_API_VERSION_1_4;

        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo =
            vk::PipelineRenderingCreateInfo{}
                .setColorAttachmentFormats({m_pVulkanLayer->getSwapchainFormatConstRef()});
        init_info.PipelineInfoForViewports = init_info.PipelineInfoMain;

        ImGui_ImplVulkan_Init(&init_info);

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    }

    void EditorLayer::releaseImGui() {
        if (const ImGuiIO &io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::DestroyPlatformWindows();
        }
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_Device.destroyDescriptorPool(m_ImGuiDescriptorPool);

        m_ImGuiDescriptorPool = nullptr;
    }

    void EditorLayer::createViewportImage(
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

        m_pRenderLayer->immediateSubmit([this](const vk::CommandBuffer cmd) {
            Vulkan::Image::TransitionLayout(m_ViewportImage.Image, vk::ImageLayout::eShaderReadOnlyOptimal, cmd);
        });

        m_ViewportImageDescriptor = ImGui_ImplVulkan_AddTexture(
            m_ViewportImageSampler,
            m_ViewportImageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void EditorLayer::destroyViewportImage() {
        ImGui_ImplVulkan_RemoveTexture(m_ViewportImageDescriptor);

        m_Device.destroySampler(m_ViewportImageSampler);
        m_Device.destroyImageView(m_ViewportImageView);

        Vulkan::Image::Destroy(m_ViewportImage, m_VmaAllocator);

        m_ViewportImageView = nullptr;
    }

    void EditorLayer::renderImGui() {
        ImGui::Begin("Ignis::EditorLayer::Toolbox");

        ImGui::ColorEdit3("Viewport Clear Color", &m_ViewportClearColor.r);

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Ignis::EditorLayer::Viewport");
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

    bool EditorLayer::onWindowCloseEvent(const WindowCloseEvent &event) const {
        (void)m_pVulkanLayer;
        Engine::Get().stop();
        return false;
    }

    bool EditorLayer::onWindowKeyEvent(const WindowKeyEvent &event) const {
        (void)m_pVulkanLayer;
        if (KeyAction::ePress != event.Action)
            return false;

        if (KeyCode::eF != event.Key)
            return false;

        if (WindowLayer *window_layer = Engine::Get().getLayer<WindowLayer>(); window_layer->isFullscreen()) {
            window_layer->makeWindowed();
        } else {
            window_layer->makeFullscreen();
        }

        return false;
    }
}  // namespace Ignis