#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Vulkan.hpp>

#include <Ignis/Render/FrameGraph.hpp>

namespace Ignis {
    class AGPUScene {
       public:
        explicit AGPUScene(const std::type_index id) : m_ID{id} {}

        virtual ~AGPUScene() = default;

        virtual void initialize(FrameGraph &frame_graph) = 0;
        virtual void release(FrameGraph &frame_graph)    = 0;

       private:
        std::type_index m_ID;

       private:
        friend class Render;
    };

    template <typename TGPUScene>
    class IGPUScene : public AGPUScene {
       public:
        IGPUScene() : AGPUScene{typeid(TGPUScene)} {}

        ~IGPUScene() override = default;
    };

}  // namespace Ignis
