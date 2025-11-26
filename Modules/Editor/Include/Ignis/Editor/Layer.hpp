#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    class ImGuiLayer;

    class EditorLayer final : public ILayer {
       public:
        struct Settings {
        };

       public:
        explicit EditorLayer(
            VulkanLayer    *vulkan_layer,
            RenderLayer    *render_layer,
            ImGuiLayer     *im_gui_layer,
            const Settings &settings);
        ~EditorLayer() override;

        void onEvent(AEvent &event) override;

        void onPreRender() override;
        void onRender() override;

       private:
        void renderImGui();

        void createViewportImage(uint32_t width, uint32_t height);
        void destroyViewportImage();

        bool onWindowCloseEvent(const WindowCloseEvent &event) const;
        bool onWindowKeyEvent(const WindowKeyEvent &event) const;

       private:
        VulkanLayer *m_pVulkanLayer;
        RenderLayer *m_pRenderLayer;
        ImGuiLayer  *m_pImGuiLayer;

        vk::Device m_Device;
        uint32_t   m_QueueFamilyIndex;
        vk::Queue  m_GraphicsQueue;
        vk::Queue  m_ComputeQueue;
        vk::Queue  m_PresentQueue;

        vma::Allocator m_VmaAllocator;

        glm::vec3 m_ViewportClearColor;

        Vulkan::Image::Allocation m_ViewportImage;

        vk::ImageView m_ViewportImageView;
        vk::Format    m_ViewportFormat;
        vk::Extent2D  m_ViewportExtent;

        vk::DescriptorSet   m_ViewportImageDescriptor;
        FrameGraph::ImageID m_ViewportImageID;
    };
}  // namespace Ignis