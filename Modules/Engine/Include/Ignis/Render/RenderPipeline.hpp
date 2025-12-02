#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Vulkan.hpp>

#include <Ignis/Render/FrameGraph.hpp>

namespace Ignis {
    class ARenderPipeline {
       public:
        explicit ARenderPipeline(const std::type_index id) : m_ID{id} {}

        virtual ~ARenderPipeline() {}

        virtual void importResources(FrameGraph &frame_graph)               = 0;
        virtual void registerResources(FrameGraph::RenderPass &render_pass) = 0;

        virtual void draw(vk::CommandBuffer command_buffer) = 0;

       private:
        std::type_index m_ID;

       private:
        friend class Render;
    };

    template <typename TRenderPipeline>
    class IRenderPipeline : public ARenderPipeline {
       public:
        IRenderPipeline() : ARenderPipeline{typeid(TRenderPipeline)} {}

        ~IRenderPipeline() override {}

        void importResources(FrameGraph &frame_graph) override {
            IGNIS_ASSERT(
                false,
                "Ignis::IRenderPipeline<{}>: User must override importResources method with their own implementation.",
                typeid(TRenderPipeline).name());
        }

        void registerResources(FrameGraph::RenderPass &render_pass) override {
            IGNIS_ASSERT(
                false,
                "Ignis::IRenderPipeline<{}>: User must override registerResources method with their own implementation.",
                typeid(TRenderPipeline).name());
        }

        void draw(const vk::CommandBuffer command_buffer) override {
            IGNIS_ASSERT(
                false,
                "Ignis::IRenderPipeline<{}>: User must override draw method with their own implementation.",
                typeid(TRenderPipeline).name());
        }
    };

}  // namespace Ignis
