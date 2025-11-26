#include <Ignis/Editor/Layer.hpp>
#include <Ignis/Editor/ImGuiLayer.hpp>

namespace Ignis {
    EditorLayer::EditorLayer(
        VulkanLayer    *vulkan_layer,
        RenderLayer    *render_layer,
        ImGuiLayer     *im_gui_layer,
        const Settings &settings) {
        DIGNIS_ASSERT(nullptr != vulkan_layer);
        DIGNIS_ASSERT(nullptr != render_layer);
        DIGNIS_ASSERT(nullptr != im_gui_layer);

        m_pVulkanLayer = vulkan_layer;
        m_pRenderLayer = render_layer;
        m_pImGuiLayer  = im_gui_layer;

        m_Device           = m_pVulkanLayer->getDevice();
        m_QueueFamilyIndex = m_pVulkanLayer->getQueueFamilyIndex();
        m_GraphicsQueue    = m_pVulkanLayer->getGraphicsQueue();
        m_ComputeQueue     = m_pVulkanLayer->getComputeQueue();
        m_PresentQueue     = m_pVulkanLayer->getPresentQueue();

        m_VmaAllocator = m_pVulkanLayer->getVmaAllocator();

        createViewportImage(1, 1);

        DIGNIS_LOG_APPLICATION_INFO("Ignis::EditorLayer Initialized");
    }

    EditorLayer::~EditorLayer() {
        DIGNIS_LOG_APPLICATION_INFO("Ignis::EditorLayer Released");

        DIGNIS_VK_CHECK(m_Device.waitIdle());

        destroyViewportImage();
    }

    void EditorLayer::onEvent(AEvent &event) {
        EventDispatcher dispatcher(event);

        dispatcher.dispatch<WindowCloseEvent>(this, &EditorLayer::onWindowCloseEvent);
        dispatcher.dispatch<WindowKeyEvent>(this, &EditorLayer::onWindowKeyEvent);
    }

    void EditorLayer::onPreRender() {
        renderImGui();

        RenderLayer::FrameData &frame = m_pRenderLayer->getCurrentFrameData();

        m_ViewportImageID = frame.FrameGraphBuilder.import_image(
            m_ViewportImage.Image,
            m_ViewportImageView,
            m_ViewportImage.Extent,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        m_pImGuiLayer->add_read_images({FrameGraph::ImageInfo{m_ViewportImageID, vk::PipelineStageFlagBits2::eFragmentShader}});
    }

    void EditorLayer::onRender() {
        if (m_pRenderLayer->shouldSkipFrame())
            return;

        RenderLayer::FrameData &frame = m_pRenderLayer->getCurrentFrameData();

        frame.FrameGraphBuilder
            .add_render_pass("Viewport pass")
            .set_color_attachment(FrameGraph::Attachment{
                m_ViewportImageID,
                vk::ClearValue{}.setColor({m_ViewportClearColor.r, m_ViewportClearColor.g, m_ViewportClearColor.b, 1.0f}),
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            })
            .execute([](const vk::CommandBuffer command_buffer) {

            });
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

        m_pRenderLayer->immediateSubmit([this](const vk::CommandBuffer cmd) {
            Vulkan::Image::TransitionLayout(m_ViewportImage.Image, vk::ImageLayout::eShaderReadOnlyOptimal, cmd);
        });

        m_ViewportImageDescriptor = ImGui_ImplVulkan_AddTexture(
            m_pImGuiLayer->getImageSampler(),
            m_ViewportImageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void EditorLayer::destroyViewportImage() {
        ImGui_ImplVulkan_RemoveTexture(m_ViewportImageDescriptor);

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

        ImGui::Image(static_cast<const VkDescriptorSet &>(m_ViewportImageDescriptor), viewport_size);

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