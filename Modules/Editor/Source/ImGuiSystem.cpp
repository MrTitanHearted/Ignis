#include <Ignis/ImGuiSystem.hpp>

namespace Ignis {
    void ImGuiSystem::initialize() {
        const vk::Device device = Vulkan::GetDevice();

        m_DescriptorPool = Vulkan::CreateDescriptorPool(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            100,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 100},
            });

        m_ImageSampler = Vulkan::CreateSampler(vk::SamplerCreateInfo{});

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(Window::GetHandle(), true);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance            = Vulkan::GetInstance();
        init_info.PhysicalDevice      = Vulkan::GetPhysicalDevice();
        init_info.Device              = device;
        init_info.QueueFamily         = Vulkan::GetQueueFamilyIndex();
        init_info.Queue               = Vulkan::GetGraphicsQueue();
        init_info.DescriptorPool      = m_DescriptorPool;
        init_info.MinImageCount       = Vulkan::GetSwapchainMinImageCount();
        init_info.ImageCount          = Vulkan::GetSwapchainImageCount();
        init_info.UseDynamicRendering = true;
        init_info.ApiVersion          = VK_API_VERSION_1_4;

        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo =
            vk::PipelineRenderingCreateInfo{}
                .setColorAttachmentFormats({Vulkan::GetSwapchainFormatConstRef()});
        init_info.PipelineInfoForViewports = init_info.PipelineInfoMain;

        ImGui_ImplVulkan_Init(&init_info);

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        DIGNIS_LOG_APPLICATION_INFO("Ignis::ImGui UISystem initialized.");
    }

    void ImGuiSystem::release() {
        DIGNIS_VK_CHECK(Vulkan::GetDevice().waitIdle());

        if (const ImGuiIO &io = ImGui::GetIO();
            io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::DestroyPlatformWindows();
        }

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        Vulkan::DestroySampler(m_ImageSampler);
        Vulkan::DestroyDescriptorPool(m_DescriptorPool);

        DIGNIS_LOG_APPLICATION_INFO("Ignis::ImGui UISystem released.");
    }

    void ImGuiSystem::begin() {
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

        Vulkan::WaitDeviceIdle();
        for (const auto &frame_image : m_FrameImages)
            ImGui_ImplVulkan_RemoveTexture(frame_image.Descriptor);

        m_FrameImages.clear();
    }

    void ImGuiSystem::end() {
        ImGui::EndFrame();
        ImGui::Render();

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void ImGuiSystem::render(FrameGraph &frame_graph) {
        FrameGraph::RenderPass imgui_pass{
            "Ignis::ImGui UISystem RenderPass",
            {1.0f, 0.0f, 0.0f, 1.0f},
        };

        gtl::vector<FrameGraph::ImageInfo> read_images{};
        read_images.reserve(m_FrameImages.size());

        for (const auto &frame_image : m_FrameImages) {
            const FrameGraph::ImageID image_id = frame_graph.getImageID(frame_image.Handle);
            read_images.push_back(FrameGraph::ImageInfo{image_id, vk::PipelineStageFlagBits2::eFragmentShader});
        }

        imgui_pass
            .readImages(read_images)
            .setColorAttachments({FrameGraph::Attachment{
                frame_graph.getSwapchainImageID(),
                vk::ClearValue{},
                vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore,
            }})
            .setExecute([](const vk::CommandBuffer command_buffer) {
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
            });

        frame_graph.addRenderPass(std::move(imgui_pass));
    }

    vk::DescriptorPool ImGuiSystem::getDescriptorPool() const {
        return m_DescriptorPool;
    }

    vk::Sampler ImGuiSystem::getImageSampler() const {
        return m_ImageSampler;
    }

    vk::DescriptorSet ImGuiSystem::addFrameImage2D(
        const vk::Image     image,
        const vk::ImageView view,
        const vk::Extent2D &extent) {
        const vk::DescriptorSet descriptor =
            ImGui_ImplVulkan_AddTexture(m_ImageSampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_FrameImages.push_back(ImageInfo{image, view, extent, descriptor});
        return descriptor;
    }
}  // namespace Ignis