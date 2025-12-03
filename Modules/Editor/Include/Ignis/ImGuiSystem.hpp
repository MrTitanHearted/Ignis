#pragma once

#include <Ignis/Engine.hpp>

namespace Ignis {
    class ImGuiSystem final : public IGUISystem {
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

        vk::DescriptorSet addImage2D(FrameGraph::ImageID id, vk::ImageView view);

        void removeImage2D(FrameGraph::ImageID id, vk::DescriptorSet set);

       private:
        vk::DescriptorPool m_DescriptorPool;

        vk::Sampler m_ImageSampler;

        std::vector<FrameGraph::ImageID> m_ImageIDs;

        gtl::flat_hash_map<FrameGraph::ImageID, size_t> m_ImageLookUp;
    };
}  // namespace Ignis