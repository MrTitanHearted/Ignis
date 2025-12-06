#include <Ignis/Editor.hpp>
#include <Ignis/ImGuiSystem.hpp>

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

        Engine &engine = Engine::GetRef();

        FrameGraph &frame_graph = engine.getFrameGraph();

        createViewportImage(engine.getUISystem<ImGuiSystem>(), 1, 1, frame_graph);

        m_pBlinnPhong = engine.addRenderModule<BlinnPhong>(BlinnPhong::Settings{
            m_ViewportImage.Format,
            m_DepthImage.Format,
        });

        m_Model = m_pBlinnPhong->loadStaticModel("Assets/Models/vanguard/flair.fbx", frame_graph);

        m_StaticInstances.push_back(m_pBlinnPhong->addStaticInstance(m_Model, {glm::mat4x4{1.0f}}, frame_graph));

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor attached");
    }

    Editor::~Editor() {
        Vulkan::WaitDeviceIdle();

        Engine &engine = Engine::GetRef();

        engine.removeRenderModule<BlinnPhong>();

        destroyViewportImage(engine.getUISystem<ImGuiSystem>(), engine.getFrameGraph());
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

        m_pBlinnPhong->updateCamera({m_Camera.getProjection(aspect), m_Camera.getView()});
    }

    void Editor::onGUI(IGUISystem *ui_system) {
        const auto im_gui = dynamic_cast<ImGuiSystem *>(ui_system);
        ImGui::Begin("Ignis::EditorLayer::Toolbox");

        if (ImGui::Button("Play")) {
            m_Play = true;
            ImGui_ImplGlfw_RestoreCallbacks(Window::GetHandle());
        }

        ImGui::SeparatorText("Adding Instances");
        static glm::vec3 add_position = glm::vec3{0.0f};
        static glm::vec3 add_rotation = glm::vec3{0.0f};
        static float     add_scale    = 1.0f;

        ImGui::DragFloat3("Position", glm::value_ptr(add_position));
        ImGui::DragFloat3("Rotation", glm::value_ptr(add_rotation));
        ImGui::DragFloat("Scale", &add_scale, 0.002f, 0.0f, 10.0f);

        if (ImGui::Button("Add Instance")) {
            const glm::mat4x4 scaled     = glm::scale(glm::mat4x4{1.0f}, glm::vec3{add_scale});
            const glm::mat4x4 rotated    = glm::toMat4(glm::quat(glm::radians(add_rotation)));  // Fixed: radians, not normalize
            const glm::mat4x4 translated = glm::translate(glm::mat4x4{1.0f}, add_position);

            const glm::mat4x4 final = translated * rotated * scaled;

            m_StaticInstances.push_back(m_pBlinnPhong->addStaticInstance(m_Model, {final}, Engine::GetRef().getFrameGraph()));
        }

        ImGui::SeparatorText("Existing Instances");

        static std::vector<BlinnPhong::StaticInstanceHandle> to_remove{};
        to_remove.clear();

        for (uint32_t i = 0; i < m_StaticInstances.size(); i++) {
            ImGui::PushID(i);  // Unique ID for each instance's widgets

            BlinnPhong::StaticInstance instance = m_pBlinnPhong->getStaticInstance(m_Model, m_StaticInstances[i]);

            glm::vec3 position{};
            glm::quat orientation{};
            glm::vec3 scale{};
            glm::vec3 skew{};
            glm::vec4 perspective{};

            glm::decompose(instance.Transform, scale, orientation, position, skew, perspective);

            glm::vec3 rotation_deg = glm::degrees(glm::eulerAngles(orientation));

            ImGui::Text("Instance %u", i);

            ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f);
            ImGui::DragFloat3("Rotation", glm::value_ptr(rotation_deg), 1.0f);
            ImGui::DragFloat("Scale", &scale.x, 0.002f, 0.0f, 10.0f);

            if (ImGui::Button("Remove Instance")) {
                to_remove.push_back(m_StaticInstances[i]);
            }

            glm::mat4x4 scaled     = glm::scale(glm::mat4x4{1.0f}, glm::vec3{scale.x});
            glm::mat4x4 rotated    = glm::toMat4(glm::quat(glm::radians(rotation_deg)));
            glm::mat4x4 translated = glm::translate(glm::mat4x4{1.0f}, position);

            instance.Transform = translated * rotated * scaled;

            m_pBlinnPhong->setStaticInstance(m_Model, m_StaticInstances[i], instance);

            ImGui::Separator();
            ImGui::PopID();
        }

        while (!to_remove.empty()) {
            Vulkan::WaitDeviceIdle();
            auto instance = to_remove.back();
            m_pBlinnPhong->removeStaticInstance(m_Model, instance);
        }

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
        FrameGraph::RenderPass editor_pass{
            "Ignis::EditorLayer::RenderPass",
            {0.0f, 1.0f, 0.0f, 1.0f},
        };

        m_pBlinnPhong->onRenderPass(editor_pass);

        editor_pass
            .setColorAttachments({FrameGraph::Attachment{
                m_ViewportImageID,
                vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f},
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
                m_pBlinnPhong->onDraw(command_buffer);
            });

        frame_graph.addRenderPass(editor_pass);
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

        m_ViewportImageID = frame_graph.importImage(
            m_ViewportImage.Handle,
            m_ViewportView,
            m_ViewportImage.Format,
            m_ViewportImage.UsageFlags,
            m_ViewportExtent,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        m_DepthImageID = frame_graph.importImage(
            m_DepthImage.Handle,
            m_DepthView,
            m_DepthImage.Format,
            m_DepthImage.UsageFlags,
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

    bool Editor::onKeyEvent(const WindowKeyEvent &event) {
        if (KeyAction::ePress != event.Action)
            return false;

        if (KeyCode::eF == event.Key) {
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