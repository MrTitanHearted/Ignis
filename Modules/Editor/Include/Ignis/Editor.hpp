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
        static bool OnWindowKeyInput(const WindowKeyEvent &event);

       public:
        explicit Editor(const Settings &settings);
        ~Editor() override;

        void onUpdate(double dt) override;

        void onGUI(IGUISystem *ui_system) override;
        void onRender(FrameGraph &frame_graph) override;

       private:
        void createViewportImage(ImGuiSystem *im_gui, uint32_t width, uint32_t height, FrameGraph &frame_graph);
        void destroyViewportImage(ImGuiSystem *im_gui, FrameGraph &frame_graph);

        bool onMouseMove(const WindowMouseMoveEvent &event);
        bool onMouseScroll(const WindowMouseScrollEvent &event);

       private:
        Vulkan::Image m_ViewportImage;
        vk::ImageView m_ViewportView;
        vk::Extent2D  m_ViewportExtent;
        vk::Format    m_ViewportFormat;

        Vulkan::Image m_DepthImage;
        vk::ImageView m_DepthView;

        vk::DescriptorSet   m_ViewportDescriptor;
        FrameGraph::ImageID m_ViewportImageID{};
        FrameGraph::ImageID m_DepthImageID{};

        glm::vec3 m_ViewportClearColor{};

        Camera m_Camera;

        BlinnPhongScene *m_pScene = nullptr;
    };
}  // namespace Ignis