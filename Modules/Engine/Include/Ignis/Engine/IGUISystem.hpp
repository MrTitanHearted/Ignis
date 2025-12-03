#pragma once

#include <Ignis/Core.hpp>
#include <Ignis/Render.hpp>

namespace Ignis {
    class IGUISystem {
       public:
        virtual ~IGUISystem() = default;

        virtual void initialize() = 0;
        virtual void release()    = 0;

        virtual void begin() = 0;
        virtual void end()   = 0;

        virtual void render(FrameGraph &frame_graph) = 0;
    };
}  // namespace Ignis