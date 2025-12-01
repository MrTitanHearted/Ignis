#pragma once

#include <Ignis/Engine.hpp>

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

        void onUI(IUISystem *ui_system) override;
        void onRender(FrameGraph &frame_graph) override;

        void createViewportImage(uint32_t width, uint32_t height);
        void destroyViewportImage();

       private:
        glm::vec3     m_ViewportClearColor;
        Vulkan::Image m_ViewportImage;
        vk::ImageView m_ViewportImageView;
        vk::Format    m_ViewportFormat;
        vk::Extent2D  m_ViewportExtent;

        vk::DescriptorSet   m_ViewportDescriptorSet;
        FrameGraph::ImageID m_ViewportImageID;
    };
}  // namespace Ignis