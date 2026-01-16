#include <Ignis/Editor.hpp>

namespace Ignis {
    bool Editor::OnWindowClose(const WindowCloseEvent &) {
        Engine::GetRef().stop();
        return false;
    }

    Editor::Editor(const Settings &settings) {
        attachCallback<WindowCloseEvent>(OnWindowClose);
        attachCallback<WindowKeyEvent>(&Editor::onKeyEvent);
        attachCallback<WindowMouseMoveEvent>(&Editor::onMouseMove);
        attachCallback<WindowMouseScrollEvent>(&Editor::onMouseScroll);
        attachCallback<WindowMouseButtonEvent>(&Editor::onMouseButtonEvent);

        m_Play = false;

        Engine &engine = Engine::GetRef();

        FrameGraph &frame_graph = engine.getFrameGraph();

        createViewportImage(engine.getUISystem<ImGuiSystem>(), 1, 1, frame_graph);

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor attached");
    }

    Editor::~Editor() {
        Vulkan::WaitDeviceIdle();

        Engine &engine = Engine::GetRef();

        FrameGraph &frame_graph = engine.getFrameGraph();

        destroyViewportImage(engine.getUISystem<ImGuiSystem>(), frame_graph);
        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor detached");
    }

    void Editor::onUpdate(const double dt) {
        if (m_Play) {
            if (Window::IsKeyDown(KeyCode::eW))
                m_Camera.processCameraMovement(Camera::Direction::eForward, static_cast<float>(dt));
            if (Window::IsKeyDown(KeyCode::eS))
                m_Camera.processCameraMovement(Camera::Direction::eBackward, static_cast<float>(dt));
            if (Window::IsKeyDown(KeyCode::eA))
                m_Camera.processCameraMovement(Camera::Direction::eLeft, static_cast<float>(dt));
            if (Window::IsKeyDown(KeyCode::eD))
                m_Camera.processCameraMovement(Camera::Direction::eRight, static_cast<float>(dt));
            if (Window::IsKeyDown(KeyCode::eQ))
                m_Camera.processCameraMovement(Camera::Direction::eWorldDown, static_cast<float>(dt));
            if (Window::IsKeyDown(KeyCode::eE))
                m_Camera.processCameraMovement(Camera::Direction::eWorldUp, static_cast<float>(dt));
        }

        const auto width  = static_cast<float>(m_ViewportImage.Extent.width);
        const auto height = static_cast<float>(m_ViewportImage.Extent.height);
        const auto aspect = width / height;

        Render::SetCamera({m_Camera.getProjection(aspect), m_Camera.getView(), m_Camera.getPosition()});
    }

    void Editor::onGUI(IGUISystem *ui_system) {
        const auto im_gui = dynamic_cast<ImGuiSystem *>(ui_system);

        ImGui::Begin("Ignis::EditorLayer::Toolbox");
        if (ImGui::Button("Play")) {
            m_Play = true;
            ImGui_ImplGlfw_RestoreCallbacks(Window::GetHandle());
        }
        RenderModelPanel(m_ModelPanel);
        ImGui::End();

        ImGui::Begin("Ignis::EditorLayer::Lights");
        RenderLightPanel(m_LightPanel);
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Ignis::EditorLayer::Viewport");
        ImGui::PopStyleVar(2);

        const ImVec2 viewport_size = ImGui::GetContentRegionAvail();

        if (static_cast<uint32_t>(viewport_size.x) != m_ViewportExtent.width ||
            static_cast<uint32_t>(viewport_size.y) != m_ViewportExtent.height) {
            Vulkan::WaitDeviceIdle();
            auto &frame_graph = Engine::GetRef().getFrameGraph();
            destroyViewportImage(im_gui, frame_graph);
            createViewportImage(im_gui, static_cast<uint32_t>(viewport_size.x), static_cast<uint32_t>(viewport_size.y), frame_graph);
        }

        ImGui::Image(static_cast<const VkDescriptorSet &>(m_ViewportDescriptor), viewport_size);

        ImGui::End();
    }

    void Editor::onRender(FrameGraph &frame_graph) {
    }

    void Editor::createViewportImage(
        ImGuiSystem   *im_gui,
        const uint32_t width,
        const uint32_t height,
        FrameGraph    &frame_graph) {
        m_ViewportExtent
            .setWidth(width)
            .setHeight(height);

        m_ViewportImage = Vulkan::AllocateImage2D(
            {},
            vma::MemoryUsage::eGpuOnly,
            {},
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst,
            m_ViewportExtent);

        m_ViewportView = Vulkan::CreateImageColorView2D(m_ViewportImage.Handle, m_ViewportImage.Format);

        m_DepthImage = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            m_ViewportExtent);

        m_DepthView = Vulkan::CreateImageDepthView2D(m_DepthImage.Handle, m_DepthImage.Format);

        Vulkan::ImmediateSubmit([this](const vk::CommandBuffer command_buffer) {
            Vulkan::BarrierMerger merger{};
            merger.putImageBarrier(
                m_ViewportImage.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead);
            merger.putImageBarrier(
                m_DepthImage.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead);
            merger.flushBarriers(command_buffer);
        });

        m_ColorImageID = frame_graph.importImage(
            m_ViewportImage.Handle,
            m_ViewportView,
            m_ViewportImage.Format,
            m_ViewportImage.Usage,
            m_ViewportExtent,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        m_DepthImageID = frame_graph.importImage(
            m_DepthImage.Handle,
            m_DepthView,
            m_DepthImage.Format,
            m_DepthImage.Usage,
            m_ViewportExtent,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::ImageLayout::eDepthAttachmentOptimal);

        m_ViewportDescriptor = im_gui->addImage2D(m_ColorImageID, m_ViewportView);

        Render::SetViewport(m_ColorImageID, m_DepthImageID);
    }

    void Editor::destroyViewportImage(ImGuiSystem *im_gui, FrameGraph &frame_graph) {
        im_gui->removeImage2D(m_ColorImageID, m_ViewportDescriptor);

        frame_graph.removeImage(m_ColorImageID);
        frame_graph.removeImage(m_DepthImageID);

        Vulkan::DestroyImageView(m_ViewportView);
        Vulkan::DestroyImage(m_ViewportImage);

        Vulkan::DestroyImageView(m_DepthView);
        Vulkan::DestroyImage(m_DepthImage);

        m_ViewportView = nullptr;
    }

    bool Editor::onKeyEvent(const WindowKeyEvent &event) {
        if (KeyAction::ePress != event.Action)
            return false;

        if (KeyCode::eF11 == event.Key) {
            if (Window::IsFullscreen())
                Window::MakeWindowed();
            else
                Window::MakeFullscreen();
        }

        if (!m_Play)
            return false;

        if (KeyCode::eEscape == event.Key &&
            CursorMode::eNormal == Window::GetCursorMode()) {
            ImGui_ImplGlfw_InstallCallbacks(Window::GetHandle());
            m_Play = false;

            const auto [mouse_x, mouse_y] = Window::GetMousePosition();

            m_LastMouseX = mouse_x;
            m_LastMouseY = mouse_y;
        }

        return false;
    }

    bool Editor::onMouseMove(const WindowMouseMoveEvent &event) {
        if (!m_Play || CursorMode::eNormal == Window::GetCursorMode())
            return false;

        const double x_offset = event.X - m_LastMouseX;
        const double y_offset = event.Y - m_LastMouseY;

        m_LastMouseX = event.X;
        m_LastMouseY = event.Y;

        m_Camera.processMouseMovement(x_offset, -y_offset);

        return false;
    }

    bool Editor::onMouseScroll(const WindowMouseScrollEvent &event) {
        if (!m_Play || CursorMode::eNormal == Window::GetCursorMode())
            return false;
        m_Camera.processMouseScroll(event.YOffset);
        return false;
    }

    bool Editor::onMouseButtonEvent(const WindowMouseButtonEvent &event) {
        if (KeyAction::ePress != event.Action)
            return false;

        if (!m_Play)
            return false;

        if (MouseButton::eLeft == event.Button) {
            const auto [x, y] = Window::GetMousePosition();

            m_LastMouseX = x;
            m_LastMouseY = y;
            Window::SetCursorMode(CursorMode::eDisabled);
        } else if (MouseButton::eRight == event.Button) {
            Window::SetCursorMode(CursorMode::eNormal);
            const auto [x, y] = Window::GetMousePosition();

            m_LastMouseX = x;
            m_LastMouseY = y;
        }

        return false;
    }
}  // namespace Ignis