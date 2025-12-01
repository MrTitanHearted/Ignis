#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    class ImGuiSystem final : public IUISystem {
       public:
        ImGuiSystem()           = default;
        ~ImGuiSystem() override = default;

        void initialize() override;
        void release() override;

        void begin() override;
        void end() override;

        void render(FrameGraph &frame_graph) override;

        vk::DescriptorPool getDescriptorPool() const;
        vk::Sampler        getImageSampler() const;

        void readImages(const vk::ArrayProxy<FrameGraph::ImageInfo> &infos);
        void writeImages(const vk::ArrayProxy<FrameGraph::ImageInfo> &infos);
        void readBuffers(const vk::ArrayProxy<FrameGraph::BufferInfo> &infos);
        void writeBuffers(const vk::ArrayProxy<FrameGraph::BufferInfo> &infos);

       private:
        vk::DescriptorPool m_DescriptorPool;

        vk::Sampler m_ImageSampler;

        gtl::vector<FrameGraph::ImageInfo>  m_ReadImages;
        gtl::vector<FrameGraph::ImageInfo>  m_WriteImages;
        gtl::vector<FrameGraph::BufferInfo> m_ReadBuffers;
        gtl::vector<FrameGraph::BufferInfo> m_WriteBuffers;
    };
}  // namespace Ignis