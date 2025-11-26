#include <Ignis/Editor/ImGuiLayer.hpp>

namespace Ignis {
    ImGuiLayer::ImGuiLayer(
        WindowLayer    *window_layer,
        VulkanLayer    *vulkan_layer,
        RenderLayer    *render_layer,
        const Settings &settings) {
        DIGNIS_ASSERT(nullptr != vulkan_layer);
        DIGNIS_ASSERT(nullptr != render_layer);

        m_pVulkanLayer = vulkan_layer;
        m_pRenderLayer = render_layer;

        const vk::Device device = m_pVulkanLayer->getDevice();

        m_DescriptorPool = Vulkan::DescriptorPool::Create(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            settings.MaxImageDescriptors,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, settings.MaxImageDescriptors},
            },
            device);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |=
            ImGuiConfigFlags_DockingEnable |
            ImGuiConfigFlags_ViewportsEnable;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(window_layer->get(), true);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance            = m_pVulkanLayer->getInstance();
        init_info.PhysicalDevice      = m_pVulkanLayer->getPhysicalDevice();
        init_info.Device              = device;
        init_info.QueueFamily         = m_pVulkanLayer->getQueueFamilyIndex();
        init_info.Queue               = m_pVulkanLayer->getGraphicsQueue();
        init_info.DescriptorPool      = m_DescriptorPool;
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

        m_ImageSampler = Vulkan::Sampler::Create(vk::SamplerCreateInfo{}, device);

        DIGNIS_LOG_APPLICATION_INFO("Ignis::ImGuiLayer Initialized");
    }

    ImGuiLayer::~ImGuiLayer() {
        const vk::Device device = m_pVulkanLayer->getDevice();
        DIGNIS_VK_CHECK(device.waitIdle());

        device.destroySampler(m_ImageSampler);

        if (const ImGuiIO &io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::DestroyPlatformWindows();
        }
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        device.destroyDescriptorPool(m_DescriptorPool);

        DIGNIS_LOG_APPLICATION_INFO("Ignis::ImGuiLayer Released");

        m_DescriptorPool = nullptr;

        m_pVulkanLayer = nullptr;
        m_pRenderLayer = nullptr;
    }

    void ImGuiLayer::onPreRender() {
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

        ImGui::Begin("Ignis::ImGuiLayer::DockSpace", nullptr, host_window_flags);
        ImGui::PopStyleVar(3);
        const ImGuiID dock_space_id = ImGui::GetID("Ignis::ImGuiLayer::DockSpace");
        ImGui::DockSpace(dock_space_id);
        ImGui::End();

        m_ReadImages.clear();
        m_WriteImages.clear();
        m_ReadBuffers.clear();
        m_WriteBuffers.clear();
    }

    void ImGuiLayer::onRender() {
        ImGui::EndFrame();
        ImGui::Render();

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        if (m_pRenderLayer->shouldSkipFrame())
            return;

        RenderLayer::FrameData &frame = m_pRenderLayer->getCurrentFrameData();

        frame.FrameGraphBuilder
            .add_render_pass("Ignis::ImGuiLayer::RenderPass")
            .read_images(m_ReadImages)
            .write_images(m_WriteImages)
            .read_buffers(m_ReadBuffers)
            .write_buffers(m_WriteBuffers)
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

    vk::DescriptorPool ImGuiLayer::getDescriptorPool() const {
        return m_DescriptorPool;
    }

    vk::Sampler ImGuiLayer::getImageSampler() const {
        return m_ImageSampler;
    }

    void ImGuiLayer::add_read_images(const vk::ArrayProxy<FrameGraph::ImageInfo> &images) {
        m_ReadImages.insert(
            std::end(m_ReadImages),
            std::begin(images),
            std::end(images));
    }

    void ImGuiLayer::add_write_images(const vk::ArrayProxy<FrameGraph::ImageInfo> &images) {
        m_WriteImages.insert(
            std::end(m_WriteImages),
            std::begin(images),
            std::end(images));
    }

    void ImGuiLayer::add_read_buffers(const vk::ArrayProxy<FrameGraph::BufferInfo> &buffers) {
        m_ReadBuffers.insert(
            std::end(m_ReadBuffers),
            std::begin(buffers),
            std::end(buffers));
    }

    void ImGuiLayer::add_write_buffers(const vk::ArrayProxy<FrameGraph::BufferInfo> &buffers) {
        m_WriteBuffers.insert(
            std::end(m_WriteBuffers),
            std::begin(buffers),
            std::end(buffers));
    }
}  // namespace Ignis