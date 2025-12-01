#include <Ignis/Editor.hpp>
#include <Ignis/ImGuiSystem.hpp>

namespace Ignis {
    bool Editor::OnWindowClose(const WindowCloseEvent &) {
        Engine::Get().stop();
        return false;
    }

    bool Editor::OnWindowKeyInput(const WindowKeyEvent &event) {
        if (KeyAction::ePress != event.Action)
            return false;

        if (KeyCode::eF == event.Key)
            if (Window::IsFullscreen())
                Window::MakeWindowed();
            else
                Window::MakeFullscreen();

        return false;
    }

    Editor::Editor(const Settings &settings) {
        attachCallback<WindowCloseEvent>(&Editor::OnWindowClose);
        attachCallback<WindowKeyEvent>(&Editor::OnWindowKeyInput);

        m_ViewportClearColor = glm::vec3{0.0f};

        createViewportImage(1, 1);

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor attached");
    }

    Editor::~Editor() {
        destroyViewportImage();
        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor detached");
    }

    void Editor::onUI(IUISystem *ui_system) {
        const auto im_gui_system = static_cast<ImGuiSystem *>(ui_system);
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
            destroyViewportImage();
            createViewportImage(viewport_size.x, viewport_size.y);
        }

        ImGui::Image(static_cast<const VkDescriptorSet &>(m_ViewportDescriptor), viewport_size);

        ImGui::End();
    }

    void Editor::onRender(FrameGraph &frame_graph) {
        const FrameGraph::ImageID viewport_image_id = frame_graph.importImage(
            m_ViewportImage.Handle,
            m_ViewportImageView,
            m_ViewportExtent,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        FrameGraph::RenderPass editor_pass{
            "Ignis::EditorLayer::RenderPass",
            {m_ViewportClearColor.r, m_ViewportClearColor.g, m_ViewportClearColor.b, 1.0f},
        };

        editor_pass
            .setColorAttachments({FrameGraph::Attachment{
                viewport_image_id,
                vk::ClearColorValue{m_ViewportClearColor.r, m_ViewportClearColor.g, m_ViewportClearColor.b, 1.0f},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            }})
            .setExecute([](const vk::CommandBuffer) {
            });

        frame_graph.addRenderPass(std::move(editor_pass));
    }

    void Editor::createViewportImage(const uint32_t width, const uint32_t height) {
        m_ViewportFormat = vk::Format::eR32G32B32A32Sfloat;

        m_ViewportExtent
            .setWidth(width)
            .setHeight(height);

        m_ViewportImage = Vulkan::AllocateImage2D(
            {},
            vma::MemoryUsage::eGpuOnly,
            {},
            m_ViewportFormat,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst,
            m_ViewportExtent);

        m_ViewportImageView = Vulkan::CreateImageColorView2D(m_ViewportImage.Handle, m_ViewportFormat);

        Render::ImmediateSubmit([this](const vk::CommandBuffer command_buffer) {
            Vulkan::BarrierMerger merger{};
            merger.put_image_barrier(
                m_ViewportImage.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::PipelineStageFlagBits2::eBottomOfPipe,
                vk::AccessFlagBits2::eMemoryWrite |
                    vk::AccessFlagBits2::eMemoryRead);
            merger.flushBarriers(command_buffer);
        });
    }

    void Editor::destroyViewportImage() {
        Vulkan::WaitDeviceIdle();

        Vulkan::DestroyImageView(m_ViewportImageView);
        Vulkan::DestroyImage(m_ViewportImage);

        m_ViewportImageView = nullptr;
    }
}  // namespace Ignis