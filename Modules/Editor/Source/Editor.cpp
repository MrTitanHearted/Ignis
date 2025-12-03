#include <Ignis/Editor.hpp>
#include <Ignis/ImGuiSystem.hpp>

namespace Ignis {
    bool Editor::OnWindowClose(const WindowCloseEvent &) {
        Engine::GetRef().stop();
        return false;
    }

    // ReSharper disable once CppDFAConstantFunctionResult
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
        attachCallback<WindowCloseEvent>(OnWindowClose);
        attachCallback<WindowKeyEvent>(OnWindowKeyInput);
        attachCallback<WindowMouseMoveEvent>(&Editor::onMouseMove);
        attachCallback<WindowMouseScrollEvent>(&Editor::onMouseScroll);

        m_ViewportClearColor = glm::vec3{0.0f};

        Engine &engine = Engine::GetRef();

        createViewportImage(engine.getUISystem<ImGuiSystem>(), 1, 1, engine.getFrameGraph());

        m_pScene = Render::GetRef().addGPUScene<BlinnPhongScene>(m_ViewportFormat, m_DepthImage.Format);

        m_pScene->loadModel("Assets/Models/vanguard/flair.fbx", engine.getFrameGraph());

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor attached");
    }

    Editor::~Editor() {
        Vulkan::WaitDeviceIdle();

        Render::GetRef().removeGPUScene<BlinnPhongScene>();

        Engine &engine = Engine::GetRef();
        destroyViewportImage(engine.getUISystem<ImGuiSystem>(), engine.getFrameGraph());
        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor detached");
    }

    void Editor::onUpdate(const double dt) {
        const float width = Window::GetWidth();
        const float height = Window::GetHeight();

        m_pScene->updateSceneCamera(BlinnPhongScene::Camera {
            m_Camera.getProjection(width / height),
            m_Camera.getView(),
        });

        if (CursorMode::eDisabled != Window::GetCursorMode())
            return;

        if (Window::IsKeyDown(KeyCode::eW))
            m_Camera.processCameraMovement(Camera::Direction::eForward, dt);
        else if (Window::IsKeyDown(KeyCode::eS))
            m_Camera.processCameraMovement(Camera::Direction::eBackward, dt);
        else if (Window::IsKeyDown(KeyCode::eA))
            m_Camera.processCameraMovement(Camera::Direction::eLeft, dt);
        else if (Window::IsKeyDown(KeyCode::eD))
            m_Camera.processCameraMovement(Camera::Direction::eRight, dt);
        else if (Window::IsKeyDown(KeyCode::eQ))
            m_Camera.processCameraMovement(Camera::Direction::eWorldDown, dt);
        else if (Window::IsKeyDown(KeyCode::eE))
            m_Camera.processCameraMovement(Camera::Direction::eWorldUp, dt);
    }

    void Editor::onGUI(IGUISystem *ui_system) {
        const auto im_gui = static_cast<ImGuiSystem *>(ui_system);
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
            Vulkan::WaitDeviceIdle();
            auto &frame_graph = Engine::GetRef().getFrameGraph();
            destroyViewportImage(im_gui, frame_graph);
            createViewportImage(im_gui, viewport_size.x, viewport_size.y, frame_graph);
        }

        ImGui::Image(static_cast<const VkDescriptorSet &>(m_ViewportDescriptor), viewport_size);

        ImGui::End();
    }

    void Editor::onRender(FrameGraph &frame_graph) {
        FrameGraph::RenderPass editor_pass{
            "Ignis::EditorLayer::RenderPass",
            {m_ViewportClearColor.r, m_ViewportClearColor.g, m_ViewportClearColor.b, 1.0f},
        };

        m_pScene->onRender(editor_pass);

        editor_pass
            .setColorAttachments({FrameGraph::Attachment{
                m_ViewportImageID,
                vk::ClearColorValue{m_ViewportClearColor.r, m_ViewportClearColor.g, m_ViewportClearColor.b, 1.0f},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            }})
            .setDepthAttachment(FrameGraph::Attachment{
                m_DepthImageID,
                vk::ClearDepthStencilValue{1.0},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
            })
            .setExecute([this](const vk::CommandBuffer command_buffer) {
                m_pScene->onDraw(command_buffer);
            });

        frame_graph.addRenderPass(std::move(editor_pass));
    }

    void Editor::createViewportImage(
        ImGuiSystem   *im_gui,
        const uint32_t width,
        const uint32_t height,
        FrameGraph    &frame_graph) {
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

        m_ViewportView = Vulkan::CreateImageColorView2D(m_ViewportImage.Handle, m_ViewportFormat);

        m_DepthImage = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            m_ViewportExtent);

        m_DepthView = Vulkan::CreateImageDepthView2D(m_DepthImage.Handle, m_DepthImage.Format);

        Render::ImmediateSubmit([this](const vk::CommandBuffer command_buffer) {
            Vulkan::BarrierMerger merger{};
            merger.put_image_barrier(
                m_ViewportImage.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead);
            merger.put_image_barrier(
                m_DepthImage.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead);
            merger.flushBarriers(command_buffer);
        });

        m_ViewportImageID = frame_graph.importImage(
            m_ViewportImage.Handle,
            m_ViewportView,
            m_ViewportFormat,
            m_ViewportExtent,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        m_DepthImageID = frame_graph.importImage(
            m_DepthImage.Handle,
            m_DepthView,
            m_DepthImage.Format,
            m_ViewportExtent,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::ImageLayout::eDepthAttachmentOptimal);

        m_ViewportDescriptor = im_gui->addImage2D(m_ViewportImageID, m_ViewportView);
    }

    void Editor::destroyViewportImage(ImGuiSystem *im_gui, FrameGraph &frame_graph) {
        im_gui->removeImage2D(m_ViewportImageID, m_ViewportDescriptor);

        frame_graph.removeImage(m_ViewportImageID);
        frame_graph.removeImage(m_DepthImageID);

        Vulkan::DestroyImageView(m_ViewportView);
        Vulkan::DestroyImage(m_ViewportImage);

        Vulkan::DestroyImageView(m_DepthView);
        Vulkan::DestroyImage(m_DepthImage);

        m_ViewportView = nullptr;
    }

    bool Editor::onMouseMove(const WindowMouseMoveEvent &event) {
        if (CursorMode::eDisabled != Window::GetCursorMode())
            return false;

        static double previous_x = event.X;
        static double previous_y = event.Y;

        const double x_offset = event.X - previous_x;
        const double y_offset = event.Y - previous_y;

        m_Camera.processMouseMovement(x_offset, -y_offset);

        return false;
    }

    bool Editor::onMouseScroll(const WindowMouseScrollEvent &event) {
        if (CursorMode::eDisabled != Window::GetCursorMode())
            return false;
        m_Camera.processMouseScroll(event.YOffset);
        return false;
    }
}  // namespace Ignis