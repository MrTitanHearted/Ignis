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

        vk::DescriptorSet addFrameImage2D(vk::Image image, vk::ImageView view, const vk::Extent2D &extent);

       private:
        struct ImageInfo {
            vk::Image     Handle;
            vk::ImageView View;
            vk::Extent2D  Extent;

            vk::DescriptorSet Descriptor;
        };

       private:
        vk::DescriptorPool m_DescriptorPool;

        vk::Sampler m_ImageSampler;

        gtl::vector<ImageInfo> m_FrameImages;
    };
}  // namespace Ignis