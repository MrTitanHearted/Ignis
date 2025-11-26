#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    class EditorLayer final : public ILayer {
       public:
        struct Settings {
        };

       public:
        explicit EditorLayer(VulkanLayer *vulkan_layer, RenderLayer *render_layer, const Settings &settings);
        ~EditorLayer() override;

        void onEvent(AEvent &event) override;

        void onPreRender() override;
        void onRender() override;

       private:
        void initializeImGui();
        void releaseImGui();

        void createViewportImage(uint32_t width, uint32_t height);
        void destroyViewportImage();

        void renderImGui();

        bool onWindowCloseEvent(const WindowCloseEvent &event) const;
        bool onWindowKeyEvent(const WindowKeyEvent &event) const;

       private:
        VulkanLayer *m_pVulkanLayer;
        RenderLayer *m_pRenderLayer;

        vk::Device m_Device;
        uint32_t   m_QueueFamilyIndex;
        vk::Queue  m_GraphicsQueue;
        vk::Queue  m_ComputeQueue;
        vk::Queue  m_PresentQueue;

        vma::Allocator m_VmaAllocator;

        vk::DescriptorPool m_ImGuiDescriptorPool;

        glm::vec3 m_ViewportClearColor;

        vk::DescriptorSet m_ViewportImageDescriptor;

        Vulkan::Image::Allocation m_ViewportImage;

        vk::Sampler   m_ViewportImageSampler;
        vk::ImageView m_ViewportImageView;
        vk::Format    m_ViewportFormat;
        vk::Extent2D  m_ViewportExtent;
    };
}  // namespace Ignis