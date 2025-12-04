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

        Engine &engine = Engine::GetRef();

        createViewportImage(engine.getUISystem<ImGuiSystem>(), 1, 1, engine.getFrameGraph());
        createGraphicsPipeline(engine.getFrameGraph());

        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor attached");
    }

    Editor::~Editor() {
        Vulkan::WaitDeviceIdle();

        Engine &engine = Engine::GetRef();
        destroyGraphicsPipeline(engine.getFrameGraph());
        destroyViewportImage(engine.getUISystem<ImGuiSystem>(), engine.getFrameGraph());
        DIGNIS_LOG_APPLICATION_INFO("Ignis::Editor detached");
    }

    void Editor::onUpdate(const double dt) {
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

        const float width  = m_ViewportImage.Extent.width;
        const float height = m_ViewportImage.Extent.height;
        const float aspect = width / height;

        const glm::mat4x4 camera_data[]{
            m_Camera.getProjection(aspect),
            m_Camera.getView(),
        };
        Vulkan::CopyMemoryToAllocation(camera_data, m_UniformBuffer.Allocation, 0, sizeof(camera_data));
    }

    void Editor::onGUI(AGUISystem *ui_system) {
        const auto im_gui = static_cast<ImGuiSystem *>(ui_system);
        ImGui::Begin("Ignis::EditorLayer::Toolbox");

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
            {0.0f, 1.0f, 0.0f, 1.0f},
        };

        editor_pass
            .readBuffers({m_VertexBufferInfo, m_IndexBufferInfo, m_UniformBufferInfo})
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
                command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
                command_buffer.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    m_PipelineLayout, 0,
                    {m_DescriptorSet}, {});
                command_buffer.bindVertexBuffers(0, {m_VertexBuffer.Handle}, {0});
                command_buffer.bindIndexBuffer(m_IndexBuffer.Handle, 0, vk::IndexType::eUint32);
                command_buffer.drawIndexed(m_IndicesCount, 1, 0, 0, 0);
            });

        frame_graph.addRenderPass(std::move(editor_pass));
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

    void Editor::createGraphicsPipeline(FrameGraph &frame_graph) {
        struct Vertex {
            glm::vec3 Position;
            glm::vec3 Color;
        };

        const FileAsset        shader_file   = FileAsset::LoadBinaryFromPath("Assets/Shaders/Triangle.spv").value();
        const std::string_view shader_source = shader_file.getContent();
        std::vector<uint32_t>  shader_code{};

        shader_code.resize(shader_source.size() / sizeof(uint32_t));
        std::memcpy(shader_code.data(), shader_source.data(), shader_source.size());

        m_DescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addUniformBuffer(0, vk::ShaderStageFlagBits::eVertex)
                .build();
        m_PipelineLayout = Vulkan::CreatePipelineLayout({}, {m_DescriptorLayout});

        m_ShaderModule = Vulkan::CreateShaderModuleFromSPV(shader_code);
        m_Pipeline =
            Vulkan::GraphicsPipelineBuilder()
                .setVertexShader("vs_main", m_ShaderModule)
                .setFragmentShader("fs_main", m_ShaderModule)
                .setVertexLayouts({Vulkan::VertexLayout{
                    vk::VertexInputBindingDescription{0, sizeof(Vertex), vk::VertexInputRate::eVertex},
                    {
                        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Position)},
                        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Color)},
                    },
                }})
                .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setCullMode(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
                .setColorAttachmentFormats({m_ViewportImage.Format})
                .setDepthAttachmentFormat(m_DepthImage.Format)
                .setDepthTest(true, vk::CompareOp::eLess)
                .setBlendingAdditive()
                .setBlendingAlphaBlended()
                .setNoMultisampling()
                .setNoBlending()
                .build(m_PipelineLayout);

        constexpr Vertex vertices[]{
            Vertex{glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
            Vertex{glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            Vertex{glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
        };

        constexpr uint32_t indices[]{0, 1, 2};

        m_IndicesCount = sizeof(indices) / sizeof(uint32_t);

        m_VertexBuffer =
            Vulkan::AllocateBuffer(
                {}, vma::MemoryUsage::eGpuOnly, {},
                sizeof(vertices),
                vk::BufferUsageFlagBits::eVertexBuffer |
                    vk::BufferUsageFlagBits::eTransferDst);
        m_IndexBuffer =
            Vulkan::AllocateBuffer(
                {}, vma::MemoryUsage::eGpuOnly, {},
                sizeof(indices),
                vk::BufferUsageFlagBits::eIndexBuffer |
                    vk::BufferUsageFlagBits::eTransferDst);
        m_UniformBuffer =
            Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped,
                vma::MemoryUsage::eCpuOnly, {},
                sizeof(glm::mat4x4) * 2,
                vk::BufferUsageFlagBits::eUniformBuffer);

        {
            const Vulkan::Buffer staging_buffer =
                Vulkan::AllocateBuffer(
                    vma::AllocationCreateFlagBits::eMapped,
                    vma::MemoryUsage::eCpuOnly, {},
                    m_VertexBuffer.Size + m_IndexBuffer.Size,
                    vk::BufferUsageFlagBits::eTransferSrc);

            Vulkan::CopyMemoryToAllocation(vertices, staging_buffer.Allocation, 0, m_VertexBuffer.Size);
            Vulkan::CopyMemoryToAllocation(indices, staging_buffer.Allocation, m_VertexBuffer.Size, m_IndexBuffer.Size);

            Render::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::CopyBufferToBuffer(
                    staging_buffer.Handle,
                    m_VertexBuffer.Handle,
                    0, 0, m_VertexBuffer.Size,
                    command_buffer);
                Vulkan::CopyBufferToBuffer(
                    staging_buffer.Handle,
                    m_IndexBuffer.Handle,
                    m_VertexBuffer.Size, 0,
                    m_IndexBuffer.Size,
                    command_buffer);
            });

            Vulkan::DestroyBuffer(staging_buffer);
        }

        m_VertexBufferInfo = FrameGraph::BufferInfo{
            frame_graph.importBuffer(m_VertexBuffer.Handle, m_VertexBuffer.UsageFlags, 0, m_VertexBuffer.Size),
            0,
            m_VertexBuffer.Size,
            vk::PipelineStageFlagBits2::eVertexInput,
        };

        m_IndexBufferInfo = FrameGraph::BufferInfo{
            frame_graph.importBuffer(m_IndexBuffer.Handle, m_IndexBuffer.UsageFlags, 0, m_IndexBuffer.Size),
            0,
            m_IndexBuffer.Size,
            vk::PipelineStageFlagBits2::eVertexInput,
        };

        m_UniformBufferInfo = FrameGraph::BufferInfo{
            frame_graph.importBuffer(m_UniformBuffer.Handle, m_UniformBuffer.UsageFlags, 0, m_UniformBuffer.Size),
            0,
            m_UniformBuffer.Size,
            vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eFragmentShader,
        };

        m_DescriptorPool = Vulkan::CreateDescriptorPool(
            {},
            100,
            {
                vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 100},
            });

        m_DescriptorSet = Vulkan::AllocateDescriptorSet(m_DescriptorLayout, m_DescriptorPool);

        Vulkan::DescriptorSetWriter()
            .writeUniformBuffer(0, m_UniformBuffer.Handle, 0, m_UniformBuffer.Size)
            .update(m_DescriptorSet);
    }

    void Editor::destroyGraphicsPipeline(FrameGraph &frame_graph) {
        Vulkan::DestroyDescriptorPool(m_DescriptorPool);

        m_DescriptorSet = nullptr;

        frame_graph.removeBuffer(m_VertexBufferInfo.Buffer);
        frame_graph.removeBuffer(m_IndexBufferInfo.Buffer);
        frame_graph.removeBuffer(m_UniformBufferInfo.Buffer);

        Vulkan::DestroyBuffer(m_VertexBuffer);
        Vulkan::DestroyBuffer(m_IndexBuffer);
        Vulkan::DestroyBuffer(m_UniformBuffer);

        Vulkan::DestroyPipeline(m_Pipeline);
        Vulkan::DestroyShaderModule(m_ShaderModule);

        Vulkan::DestroyPipelineLayout(m_PipelineLayout);
        Vulkan::DestroyDescriptorSetLayout(m_DescriptorLayout);
    }

    bool Editor::onMouseMove(const WindowMouseMoveEvent &event) {
        static double previous_x = event.X;
        static double previous_y = event.Y;

        const double x_offset = event.X - previous_x;
        const double y_offset = event.Y - previous_y;

        previous_x = event.X;
        previous_y = event.Y;

        m_Camera.processMouseMovement(x_offset, -y_offset);

        return false;
    }

    bool Editor::onMouseScroll(const WindowMouseScrollEvent &event) {
        m_Camera.processMouseScroll(event.YOffset);
        return false;
    }
}  // namespace Ignis