#pragma once

#include <Ignis/Engine.hpp>

#include <Ignis/ImGuiSystem.hpp>
#include <Ignis/BlinnPhong.hpp>

namespace Ignis {
    class Editor final : public Layer<Editor> {
       public:
        struct Settings {
        };

       public:
        static bool OnWindowClose(const WindowCloseEvent &);

       public:
        explicit Editor(const Settings &settings);
        ~Editor() override;

        void onUpdate(double dt) override;

        void onGUI(IGUISystem *ui_system) override;
        void onRender(FrameGraph &frame_graph) override;

       private:
        void createViewportImage(ImGuiSystem *im_gui, uint32_t width, uint32_t height, FrameGraph &frame_graph);
        void destroyViewportImage(ImGuiSystem *im_gui, FrameGraph &frame_graph);

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
        FrameGraph::ImageID m_ViewportImageID{FrameGraph::k_InvalidImageID};
        FrameGraph::ImageID m_DepthImageID{FrameGraph::k_InvalidImageID};

        BlinnPhong *m_pBlinnPhong;

        BlinnPhong::StaticModelHandle m_Model;
    };
}  // namespace Ignis