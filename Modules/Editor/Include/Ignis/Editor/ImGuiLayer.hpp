#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    class ImGuiLayer final : public ILayer {
       public:
        struct Settings {
            uint32_t MaxImageDescriptors = 100;
        };

       public:
        ImGuiLayer(
            WindowLayer    *window_layer,
            VulkanLayer    *vulkan_layer,
            RenderLayer    *render_layer,
            const Settings &settings);
        ~ImGuiLayer() override;

        void onPreRender() override;
        void onRender() override;

        vk::DescriptorPool getDescriptorPool() const;
        vk::Sampler        getImageSampler() const;

        void add_read_images(const vk::ArrayProxy<FrameGraph::ImageInfo> &images);
        void add_write_images(const vk::ArrayProxy<FrameGraph::ImageInfo> &images);
        void add_read_buffers(const vk::ArrayProxy<FrameGraph::BufferInfo> &buffers);
        void add_write_buffers(const vk::ArrayProxy<FrameGraph::BufferInfo> &buffers);

       private:
        VulkanLayer *m_pVulkanLayer;
        RenderLayer *m_pRenderLayer;

        vk::DescriptorPool m_DescriptorPool;
        vk::Sampler        m_ImageSampler;

        gtl::vector<FrameGraph::ImageInfo>  m_ReadImages;
        gtl::vector<FrameGraph::ImageInfo>  m_WriteImages;
        gtl::vector<FrameGraph::BufferInfo> m_ReadBuffers;
        gtl::vector<FrameGraph::BufferInfo> m_WriteBuffers;
    };
}  // namespace Ignis