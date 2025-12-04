#pragma once

#include <Ignis/Engine.hpp>

#include <Ignis/ImGuiSystem.hpp>

namespace Ignis {
    class Editor final : public ILayer<Editor> {
       public:
        struct Settings {
        };

       public:
        static bool OnWindowClose(const WindowCloseEvent &);

       public:
        explicit Editor(const Settings &settings);
        ~Editor() override;

        void onUpdate(double dt) override;

        void onGUI(AGUISystem *ui_system) override;
        void onRender(FrameGraph &frame_graph) override;

       private:
        void createViewportImage(ImGuiSystem *im_gui, uint32_t width, uint32_t height, FrameGraph &frame_graph);
        void destroyViewportImage(ImGuiSystem *im_gui, FrameGraph &frame_graph);

        void createGraphicsPipeline(FrameGraph &frame_graph);
        void destroyGraphicsPipeline(FrameGraph &frame_graph);

        bool onKeyEvent(const WindowKeyEvent &event);
        bool onMouseMove(const WindowMouseMoveEvent &event);
        bool onMouseScroll(const WindowMouseScrollEvent &event);
        bool onMouseButtonEvent(const WindowMouseButtonEvent &event);

       private:
        float  m_LastMouseX;
        float  m_LastMouseY;
        Camera m_Camera;

        bool m_Play;

        Vulkan::Image m_ViewportImage;
        vk::ImageView m_ViewportView;
        vk::Extent2D  m_ViewportExtent;

        Vulkan::Image m_DepthImage;
        vk::ImageView m_DepthView;

        vk::DescriptorSet   m_ViewportDescriptor;
        FrameGraph::ImageID m_ViewportImageID{};
        FrameGraph::ImageID m_DepthImageID{};

        vk::DescriptorSetLayout m_DescriptorLayout;
        vk::PipelineLayout      m_PipelineLayout;

        vk::ShaderModule m_ShaderModule;
        vk::Pipeline     m_Pipeline;

        uint32_t m_IndicesCount;

        Vulkan::Buffer m_VertexBuffer;
        Vulkan::Buffer m_IndexBuffer;
        Vulkan::Buffer m_UniformBuffer;

        FrameGraph::BufferInfo m_VertexBufferInfo;
        FrameGraph::BufferInfo m_IndexBufferInfo;
        FrameGraph::BufferInfo m_UniformBufferInfo;

        vk::DescriptorPool m_DescriptorPool;
        vk::DescriptorSet  m_DescriptorSet;
    };
}  // namespace Ignis